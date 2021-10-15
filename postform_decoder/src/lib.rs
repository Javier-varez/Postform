use byteorder::{LittleEndian, ReadBytesExt};
use colored::Colorize;
use object::read::{File as ElfFile, Object, ObjectSection, ObjectSymbol};
use std::{fs, path::Path};

include!(concat!(env!("OUT_DIR"), "/version.rs"));

/// Error type for Postform Decoder.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("Postform IO error")]
    IoError {
        #[from]
        #[source]
        io_err: std::io::Error,
    },
    #[error("Error handling ELF data")]
    ElfParseError {
        #[from]
        #[source]
        source: object::read::Error,
    },
    #[error("No interned strings found")]
    MissingInternedStrings,
    #[error("No postform configuration found")]
    MissingPostformConfiguration,
    #[error("Missing Postform version")]
    MissingPostformVersion,
    #[error("Mismatched Postform versions. Firmware: {0}, Host: {1}")]
    MismatchedPostformVersions(String, String),
    #[error("Log Level not found")]
    LevelNotFound,
    #[error("Invalid format string")]
    InvalidFormatString,
    #[error("Invalid log message")]
    InvalidLogMessage,
    #[error("Missing log argument")]
    MissingLogArgument,
    #[error("Invalid format specifier: '{0}'")]
    InvalidFormatSpecifier(char),
}

/// Available log levels of Postform.
#[derive(Copy, Clone, Debug, strum_macros::ToString)]
pub enum LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Unknown,
}

impl LogLevel {
    fn get_start_section_label(&self) -> String {
        format!("__Interned{}Start", self.to_string())
    }
    fn get_end_section_label(&self) -> String {
        format!("__Interned{}End", self.to_string())
    }

    fn find_level_in_elf(&self, elf_file: &ElfFile) -> Result<(usize, usize), Error> {
        let start = elf_file
            .symbols()
            .find(|x| x.name().unwrap() == self.get_start_section_label())
            .map(|section| section.address() as usize)
            .ok_or(Error::LevelNotFound)?;

        let end = elf_file
            .symbols()
            .find(|x| x.name().unwrap() == self.get_end_section_label())
            .map(|section| section.address() as usize)
            .ok_or(Error::LevelNotFound)?;

        Ok((start, end))
    }
}

struct LogSection {
    level: LogLevel,
    start: usize,
    end: usize,
}

/// Representation of a parsed Postform log.
pub struct Log {
    pub timestamp: f64,
    pub level: LogLevel,
    pub message: String,
    pub file_name: String,
    pub line_number: u32,
}

/// The ElfMetadata struct encapsulates all log metadata contained in the target ELF file.
/// The log metadata contains the target configuration, along with the interned strings and
/// log section markers.
///
/// An instance of ElfMetadata is required in order to parse any logs received from the target.
///
/// ```
/// use std::path::Path;
/// use postform_decoder::{Decoder, ElfMetadata};
/// fn postform_example(file: &Path) {
///     let elf_metadata = ElfMetadata::from_elf_file(file, false).unwrap();
///
///     // Get postform messages from the device
///     // This is just an example buffer, assume it comes from the target device
///     let message = [0u8; 12];
///
///     // Parse the logs using the elf metadata
///     let mut decoder = Decoder::new(&elf_metadata);
///     let log = decoder.decode(&message).unwrap();
///     println!("{}: {}", log.timestamp, log.message);
/// }
/// ```
pub struct ElfMetadata {
    timestamp_freq: f64,
    strings: Vec<u8>,
    log_sections: Vec<LogSection>,
}

impl ElfMetadata {
    /// Attempts to instantiate the ElfMetadata struct from the provided ELF file.
    pub fn from_elf_file(elf_path: &Path, disable_version_check: bool) -> Result<Self, Error> {
        let file_contents = fs::read(elf_path)?;
        let elf_file = ElfFile::parse(&file_contents[..])?;

        let postform_version = elf_file
            .section_by_name(".postform_version")
            .ok_or(Error::MissingPostformVersion)?
            .data()?;

        let postform_version = String::from_utf8_lossy(
            &postform_version[..postform_version
                .iter()
                .position(|&c| c == b'\0')
                .unwrap_or(postform_version.len())],
        );
        if !disable_version_check && postform_version != POSTFORM_VERSION {
            return Err(Error::MismatchedPostformVersions(
                postform_version.to_string(),
                POSTFORM_VERSION.to_string(),
            ));
        }
        let interned_strings = elf_file
            .section_by_name(".interned_strings")
            .ok_or(Error::MissingInternedStrings)?
            .data()?;
        let timestamp_freq = elf_file
            .section_by_name(".postform_config")
            .ok_or(Error::MissingPostformConfiguration)?
            .data()?
            .read_u32::<LittleEndian>()? as f64;

        let levels = [
            LogLevel::Debug,
            LogLevel::Info,
            LogLevel::Warning,
            LogLevel::Error,
        ];
        let mut sections = vec![];
        for level in &levels {
            if let Ok((start, end)) = level.find_level_in_elf(&elf_file) {
                sections.push(LogSection {
                    level: *level,
                    start,
                    end,
                });
            } else {
                log::warn!("Log level {:?} not found in elf file", level);
            }
        }

        Ok(Self {
            timestamp_freq,
            strings: interned_strings.into(),
            log_sections: sections,
        })
    }

    fn get_log_section(&self, str_ptr: usize) -> &LogSection {
        let log_section = self
            .log_sections
            .iter()
            .find(|&x| str_ptr >= x.start && str_ptr < x.end);

        match log_section {
            Some(section) => section,
            None => &LogSection {
                level: LogLevel::Unknown,
                start: 0usize,
                end: 0usize,
            },
        }
    }

    fn recover_interned_string(&self, str_ptr: usize) -> Result<String, Error> {
        let str_buffer = &self.strings[str_ptr..];
        let end_of_string = str_buffer
            .iter()
            .position(|&c| c == b'\0')
            .ok_or(Error::InvalidFormatString)?;
        Ok(String::from_utf8_lossy(&str_buffer[..end_of_string]).to_string())
    }
}

type FormatSpecHandler = for<'a> fn(&Decoder, &mut String, &'_ mut &'a [u8]) -> Result<(), Error>;

fn decode_unsigned(message: &'_ mut &'_ [u8]) -> Result<u64, Error> {
    leb128::read::unsigned(message).map_err(|_| Error::InvalidLogMessage)
}

fn decode_signed(message: &'_ mut &'_ [u8]) -> Result<i64, Error> {
    leb128::read::signed(message).map_err(|_| Error::InvalidLogMessage)
}

fn format_unsigned<'a>(out_str: &mut String, buffer: &'_ mut &'a [u8]) -> Result<(), Error> {
    let integer_val = decode_unsigned(buffer)?;
    let integer_str = format!("{}", integer_val);
    out_str.push_str(&integer_str);
    Ok(())
}

fn format_signed<'a>(out_str: &mut String, buffer: &'_ mut &'a [u8]) -> Result<(), Error> {
    let integer_val = decode_signed(buffer)?;
    let integer_str = format!("{}", integer_val);
    out_str.push_str(&integer_str);
    Ok(())
}

fn format_octal<'a>(out_str: &mut String, buffer: &'_ mut &'a [u8]) -> Result<(), Error> {
    let integer_val = decode_unsigned(buffer)?;
    let integer_str = format!("{:o}", integer_val);
    out_str.push_str(&integer_str);
    Ok(())
}

fn format_hex<'a>(out_str: &mut String, buffer: &'_ mut &'a [u8]) -> Result<(), Error> {
    let integer_val = decode_unsigned(buffer)?;
    let integer_str = format!("{:x}", integer_val);
    out_str.push_str(&integer_str);
    Ok(())
}

fn format_pointer<'a>(out_str: &mut String, buffer: &'_ mut &'a [u8]) -> Result<(), Error> {
    let integer_val = decode_unsigned(buffer)?;
    let integer_str = format!("0x{:x}", integer_val);
    out_str.push_str(&integer_str);
    Ok(())
}

const FORMAT_SPEC_TABLE: [(&str, FormatSpecHandler); 24] = [
    ("%s", |_, out_str, buffer| {
        let nul_range_end = buffer
            .iter()
            .position(|&c| c == b'\0')
            .unwrap_or(buffer.len());

        let res =
            std::str::from_utf8(&buffer[..nul_range_end]).or(Err(Error::MissingLogArgument))?;
        *buffer = &buffer[nul_range_end + 1..];
        out_str.push_str(res);
        Ok(())
    }),
    ("%hhd", |_, out_str, buffer| format_signed(out_str, buffer)),
    ("%hd", |_, out_str, buffer| format_signed(out_str, buffer)),
    ("%d", |_, out_str, buffer| format_signed(out_str, buffer)),
    ("%ld", |_, out_str, buffer| format_signed(out_str, buffer)),
    ("%lld", |_, out_str, buffer| format_signed(out_str, buffer)),
    ("%hhu", |_, out_str, buffer| {
        format_unsigned(out_str, buffer)
    }),
    ("%hu", |_, out_str, buffer| format_unsigned(out_str, buffer)),
    ("%u", |_, out_str, buffer| format_unsigned(out_str, buffer)),
    ("%lu", |_, out_str, buffer| format_unsigned(out_str, buffer)),
    ("%llu", |_, out_str, buffer| {
        format_unsigned(out_str, buffer)
    }),
    ("%hho", |_, out_str, buffer| format_octal(out_str, buffer)),
    ("%ho", |_, out_str, buffer| format_octal(out_str, buffer)),
    ("%o", |_, out_str, buffer| format_octal(out_str, buffer)),
    ("%lo", |_, out_str, buffer| format_octal(out_str, buffer)),
    ("%llo", |_, out_str, buffer| format_octal(out_str, buffer)),
    ("%hhx", |_, out_str, buffer| format_hex(out_str, buffer)),
    ("%hx", |_, out_str, buffer| format_hex(out_str, buffer)),
    ("%x", |_, out_str, buffer| format_hex(out_str, buffer)),
    ("%lx", |_, out_str, buffer| format_hex(out_str, buffer)),
    ("%llx", |_, out_str, buffer| format_hex(out_str, buffer)),
    ("%p", |_, out_str, buffer| format_pointer(out_str, buffer)),
    ("%k", |decoder, out_str, buffer| {
        let str_ptr = decode_unsigned(buffer)? as usize;
        let interned_string = decoder
            .elf_metadata
            .recover_interned_string(str_ptr as usize)?;
        out_str.push_str(&interned_string);
        Ok(())
    }),
    ("%%", |_, out_str, _| {
        out_str.push('%');
        Ok(())
    }),
];

/// Decodes Postform logs from the ElfMetadata and a buffer.
pub struct Decoder<'a> {
    elf_metadata: &'a ElfMetadata,
}

impl<'a> Decoder<'a> {
    /// Creates a new Decoder that uses the borrowed ElfMetadata.
    pub fn new(elf_metadata: &'a ElfMetadata) -> Self {
        Decoder { elf_metadata }
    }

    /// Parses a Postform message from the passed buffer.
    /// If the buffer is invalid it may return an error.
    pub fn decode(&mut self, mut buffer: &[u8]) -> Result<Log, Error> {
        let timestamp = leb128::read::unsigned(&mut buffer).map_err(|_| Error::InvalidLogMessage)?
            as f64
            / self.elf_metadata.timestamp_freq;

        let str_ptr = decode_unsigned(&mut buffer)? as usize;

        let format_string = self.elf_metadata.recover_interned_string(str_ptr)?;
        let (file_name, line_number, format_str) = self.decode_format_string(format_string)?;
        let formatted_str = self.format_string(&format_str, buffer)?;
        let log_section = self.elf_metadata.get_log_section(str_ptr);

        Ok(Log {
            timestamp,
            level: log_section.level,
            message: formatted_str,
            file_name,
            line_number,
        })
    }

    fn decode_format_string(
        &self,
        interned_string: String,
    ) -> Result<(String, u32, String), Error> {
        let mut splits = interned_string.split('@');

        let file_name = splits.next().ok_or(Error::InvalidFormatString)?.to_owned();
        let line_number = splits
            .next()
            .ok_or(Error::InvalidFormatString)?
            .parse()
            .or(Err(Error::InvalidFormatString))?;
        let format = splits.next().ok_or(Error::InvalidFormatString)?.to_owned();

        Ok((file_name, line_number, format))
    }

    fn format_string(&self, format: &str, mut arguments: &[u8]) -> Result<String, Error> {
        let mut format = String::from(format);
        let mut formatted_str = String::new();
        loop {
            let format_spec_pos = match format.find('%') {
                Some(pos) => pos,
                None => {
                    // Insert what's left of the format string and return;
                    formatted_str.push_str(&format);
                    return Ok(formatted_str);
                }
            };

            // Push characters until the %
            format
                .chars()
                .take(format_spec_pos)
                .for_each(|c| formatted_str.push(c));
            // Advance the format string
            format = format.chars().skip(format_spec_pos).collect();

            for (format_spec, handler) in &FORMAT_SPEC_TABLE {
                if format.starts_with(format_spec) {
                    handler(self, &mut formatted_str, &mut arguments)?;
                    // Advance the format string past the format specifier
                    format = format.chars().skip(format_spec.len()).collect();
                    break;
                }
            }
        }
    }
}

/// Decoder for logs sent through a SerialLogger in libpostform. Implements rcobs decoding and
/// detection of start/end of message.
pub struct SerialDecoder<'a> {
    rcobs_msg_buffer: Vec<u8>,
    decoder: Decoder<'a>,
}

impl<'a> SerialDecoder<'a> {
    /// Creates a new SerialDecoder instance.
    pub fn new(elf_metadata: &'a ElfMetadata) -> Self {
        Self {
            rcobs_msg_buffer: vec![],
            decoder: Decoder::new(elf_metadata),
        }
    }

    /// Feeds data to the decoder and accepts an action to trigger every time a message is decoded.
    /// This action can then print the logs to stdout or do whatever the user prefers.
    pub fn feed_and_do<T>(&mut self, data: &[u8], action: T)
    where
        T: Fn(Log),
    {
        for byte in data {
            if *byte == 0 {
                match rcobs::decode(&self.rcobs_msg_buffer[..]) {
                    Ok(message) => {
                        match self.decoder.decode(&message[..]) {
                            Ok(log) => {
                                action(log);
                            }
                            Err(error) => {
                                println!("{}{}", "Error parsing log: ".red(), error);
                            }
                        };
                    }
                    Err(_) => {
                        println!("{}", "Error decoding rcobs log".red());
                    }
                }
                self.rcobs_msg_buffer.clear();
            } else {
                self.rcobs_msg_buffer.push(*byte);
            }
        }
    }
}

/// Returns the associated color for the log level
fn color_for_level(level: LogLevel) -> colored::Color {
    match level {
        LogLevel::Debug => colored::Color::Green,
        LogLevel::Info => colored::Color::Yellow,
        LogLevel::Warning => colored::Color::TrueColor {
            r: 0xFFu8,
            g: 0xA5u8,
            b: 0u8,
        },
        LogLevel::Error => colored::Color::Red,
        LogLevel::Unknown => colored::Color::Red,
    }
}

/// Reads a log from buffer and prints it to stdout
pub fn print_log(log: &Log) {
    println!(
        "{timestamp:<12.6} {level:<11}: {msg}",
        timestamp = log.timestamp,
        level = log.level.to_string().color(color_for_level(log.level)),
        msg = log.message
    );
    println!(
        "{}{}{}{}",
        "└── File: ".dimmed(),
        log.file_name.dimmed(),
        ", Line number: ".dimmed(),
        log.line_number.to_string().dimmed(),
    );
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_elf_metadata() -> ElfMetadata {
        ElfMetadata {
            timestamp_freq: 1_000f64,
            strings: b"test/my_file.cpp@1234@This is my log message\0test/my_file2.cpp@12343@This is my second log message\0".into_iter().map(|c| c.clone()).collect(),
            log_sections: vec![],
        }
    }

    #[test]
    fn test_recover_interned_string() {
        let elf_metadata = create_elf_metadata();
        let format_string = elf_metadata.recover_interned_string(45usize).unwrap();
        let (file_name, line, msg) = Decoder::new(&elf_metadata)
            .decode_format_string(format_string)
            .unwrap();
        assert_eq!(file_name, "test/my_file2.cpp");
        assert_eq!(line, 12343u32);
        assert_eq!(msg, "This is my second log message");
    }

    #[test]
    fn test_format_string_signed_integer() {
        let elf_metadata = create_elf_metadata();
        let decoder = Decoder::new(&elf_metadata);
        let format = "This is the log message %d and some data after";
        let mut args = [0u8; 5];
        let mut args_slice = &mut args[..];
        leb128::write::signed(&mut args_slice, -4000230).expect("Couldn't format arg");
        let log = decoder.format_string(format, &args).unwrap();
        assert_eq!(log, "This is the log message -4000230 and some data after");
    }

    #[test]
    fn test_format_string_unsigned_integer() {
        let elf_metadata = create_elf_metadata();
        let decoder = Decoder::new(&elf_metadata);
        let format = "This is the log message %u";
        let mut args = [0u8; 5];
        let mut args_slice = &mut args[..];
        leb128::write::unsigned(&mut args_slice, 4000230).expect("Couldn't format arg");
        let log = decoder.format_string(format, &args).unwrap();
        assert_eq!(log, "This is the log message 4000230");
    }

    #[test]
    fn test_format_string_string_argument() {
        let elf_metadata = create_elf_metadata();
        let decoder = Decoder::new(&elf_metadata);
        let format = "This is the log message %s";
        let args = b"And another string goes here\0 some other data";
        let log = decoder.format_string(format, args).unwrap();
        assert_eq!(log, "This is the log message And another string goes here");
    }
}
