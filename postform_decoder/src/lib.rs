mod shared_types;

use byteorder::{LittleEndian, ReadBytesExt};
use object::read::{File as ElfFile, Object, ObjectSection, ObjectSymbol};
use shared_types::Postform_PlatformDescription as PostformPlatformDescription;
use std::{fs, path::PathBuf};

include!(concat!(env!("OUT_DIR"), "/version.rs"));

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
    #[error("No postform platform description found")]
    MissingPostformPlatformDescription,
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
    #[error("Platform descriptors are not valid: '{0:?}'")]
    InvalidPlatformDescriptors(PostformPlatformDescription),
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
/// use std::path::PathBuf;
/// use postform_decoder::{Decoder, ElfMetadata};
/// fn postform_example(file: &PathBuf) {
///     let elf_metadata = ElfMetadata::from_elf_file(file).unwrap();
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
    platform_descriptors: PostformPlatformDescription,
}

impl ElfMetadata {
    /// Attempts to instantiate the ElfMetadata struct from the provided ELF file.
    pub fn from_elf_file(elf_path: &PathBuf) -> Result<Self, Error> {
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
        if postform_version != POSTFORM_VERSION {
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
        let postform_platform_descriptors_data = elf_file
            .section_by_name(".postform_platform_descriptors")
            .ok_or(Error::MissingPostformPlatformDescription)?
            .data()?;
        let mut postform_platform_descriptors =
            std::mem::MaybeUninit::<PostformPlatformDescription>::uninit();
        unsafe {
            std::ptr::copy_nonoverlapping(
                postform_platform_descriptors_data.as_ptr(),
                postform_platform_descriptors.as_mut_ptr() as *mut u8,
                std::mem::size_of::<PostformPlatformDescription>(),
            );
        };

        // SAFETY: At this point, the platform descriptors should be correctly initialized, since
        // it has been copied from the ELF section.
        let postform_platform_descriptors = unsafe { postform_platform_descriptors.assume_init() };

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
                println!("Warning: Level {:?} not found in elf file", level);
            }
        }

        Ok(Self {
            timestamp_freq,
            strings: interned_strings.into(),
            log_sections: sections,
            platform_descriptors: postform_platform_descriptors,
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

    fn recover_interned_string(&self, str_ptr: usize) -> Result<(String, u32, String), Error> {
        let str_buffer = &self.strings[str_ptr..];
        let end_of_string = str_buffer
            .iter()
            .position(|&c| c == b'\0')
            .ok_or(Error::InvalidFormatString)?;
        let str_buffer = &str_buffer[..end_of_string];

        let at_pos = str_buffer
            .iter()
            .position(|&c| c == b'@')
            .ok_or(Error::InvalidFormatString)?;
        let file_name = String::from_utf8_lossy(&str_buffer[..at_pos]).to_string();
        let str_buffer = &str_buffer[at_pos + 1..];

        let at_pos = str_buffer
            .iter()
            .position(|&c| c == b'@')
            .ok_or(Error::InvalidFormatString)?;
        let line_number = String::from_utf8_lossy(&str_buffer[..at_pos])
            .parse()
            .or(Err(Error::InvalidFormatString))?;
        let format = String::from_utf8_lossy(&str_buffer[at_pos + 1..]).to_string();

        Ok((file_name, line_number, format))
    }

    fn recover_user_interned_string(&self, str_ptr: usize) -> Result<String, Error> {
        let str_buffer = &self.strings[str_ptr..];
        let end_of_string = str_buffer
            .iter()
            .position(|&c| c == b'\0')
            .ok_or(Error::InvalidFormatString)?;
        Ok(String::from_utf8_lossy(&str_buffer[..end_of_string]).to_string())
    }
}

type FormatSpecHandler = for<'a> fn(&Decoder, &mut String, &'_ mut &'a [u8]) -> Result<(), Error>;

const FORMAT_SPEC_TABLE: [(&str, FormatSpecHandler); 5] = [
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
    ("%d", |_, out_str, buffer| {
        let integer_val = buffer.read_i32::<LittleEndian>()?;
        let integer_str = format!("{}", integer_val);
        out_str.push_str(&integer_str);
        Ok(())
    }),
    ("%u", |_, out_str, buffer| {
        let integer_val = buffer.read_u32::<LittleEndian>()?;
        let integer_str = format!("{}", integer_val);
        out_str.push_str(&integer_str);
        Ok(())
    }),
    ("%k", |decoder, out_str, buffer| {
        let str_ptr = decoder.decode_pointer(buffer)?;
        let interned_string = decoder
            .elf_metadata
            .recover_user_interned_string(str_ptr as usize)?;
        out_str.push_str(&interned_string);
        Ok(())
    }),
    ("%%", |_, out_str, _| {
        out_str.push('%');
        Ok(())
    }),
];

pub struct Decoder<'a> {
    elf_metadata: &'a ElfMetadata,
}

impl<'a> Decoder<'a> {
    pub fn new(elf_metadata: &'a ElfMetadata) -> Self {
        Decoder { elf_metadata }
    }

    /// Parses a Postform message from the passed buffer.
    /// If the buffer is invalid it may return an error.
    pub fn decode(&mut self, mut buffer: &[u8]) -> Result<Log, Error> {
        let timestamp =
            buffer.read_u64::<byteorder::LittleEndian>()? as f64 / self.elf_metadata.timestamp_freq;

        let str_ptr = self.decode_pointer(&mut buffer)?;

        let (file_name, line_number, format_str) = self
            .elf_metadata
            .recover_interned_string(str_ptr as usize)?;
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
                    handler(&self, &mut formatted_str, &mut arguments)?;
                    // Advance the format string past the format specifier
                    format = format.chars().skip(format_spec.len()).collect();
                    break;
                }
            }
        }
    }

    fn decode_pointer<'b>(&self, message: &'_ mut &'b [u8]) -> Result<usize, Error> {
        match self.elf_metadata.platform_descriptors.ptr_size {
            1 => message.read_u8().map(|x| x as usize),
            2 => message.read_u16::<LittleEndian>().map(|x| x as usize),
            4 => message.read_u32::<LittleEndian>().map(|x| x as usize),
            8 => message.read_u64::<LittleEndian>().map(|x| x as usize),
            _ => panic!(
                "Invalid platform pointer size: {}",
                self.elf_metadata.platform_descriptors.ptr_size
            ),
        }
        .or(Err(Error::InvalidLogMessage))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_elf_metadata() -> ElfMetadata {
        ElfMetadata {
            timestamp_freq: 1_000f64,
            strings: b"test/my_file.cpp@1234@This is my log message\0test/my_file2.cpp@12343@This is my second log message\0".into_iter().map(|c| c.clone()).collect(),
            log_sections: vec![],
            platform_descriptors: PostformPlatformDescription {
                char_size: 1,
                short_size: 2,
                int_size: 4,
                long_int_size: 8,
                long_long_int_size: 8,
                ptr_size: 8
            }
        }
    }

    #[test]
    fn test_recover_interned_string() {
        let elf_metadata = create_elf_metadata();
        let (file_name, line, msg) = elf_metadata.recover_interned_string(45usize).unwrap();
        assert_eq!(file_name, "test/my_file2.cpp");
        assert_eq!(line, 12343u32);
        assert_eq!(msg, "This is my second log message");
    }

    #[test]
    fn test_format_string_signed_integer() {
        let elf_metadata = create_elf_metadata();
        let decoder = Decoder::new(&elf_metadata);
        let format = "This is the log message %d and some data after";
        let args = (-4_000_230i32).to_le_bytes();
        let log = decoder.format_string(format, &args).unwrap();
        assert_eq!(log, "This is the log message -4000230 and some data after");
    }

    #[test]
    fn test_format_string_unsigned_integer() {
        let elf_metadata = create_elf_metadata();
        let decoder = Decoder::new(&elf_metadata);
        let format = "This is the log message %u";
        let args = 4_000_230u32.to_le_bytes();
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
