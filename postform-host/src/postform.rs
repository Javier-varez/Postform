use byteorder::{LittleEndian, ReadBytesExt};
use object::read::{File as ElfFile, Object, ObjectSection, ObjectSymbol};
use std::{convert::TryInto, fs, path::PathBuf};

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

    fn find_level_in_elf(&self, elf_file: &ElfFile) -> Result<(u64, u64), Error> {
        let start = elf_file
            .symbols()
            .find(|x| x.name().unwrap() == self.get_start_section_label())
            .map(|section| section.address())
            .ok_or(Error::LevelNotFound)?;

        let end = elf_file
            .symbols()
            .find(|x| x.name().unwrap() == self.get_end_section_label())
            .map(|section| section.address())
            .ok_or(Error::LevelNotFound)?;

        Ok((start, end))
    }
}

struct LogSection {
    level: LogLevel,
    start: u64,
    end: u64,
}

/// Representation of a parsed Postform log.
pub struct Log {
    pub timestamp: f64,
    pub level: LogLevel,
    pub message: String,
    pub file_name: String,
    pub line_number: String,
}

/// The ElfMetadata struct encapsulates all log metadata contained in the target ELF file.
/// The log metadata contains the target configuration, along with the interned strings and
/// log section markers.
///
/// An instance of ElfMetadata is required in order to parse any logs received from the target.
pub struct ElfMetadata {
    timestamp_freq: f64,
    strings: Vec<u8>,
    log_sections: Vec<LogSection>,
}

impl ElfMetadata {
    /// Attempts to instantiate the ElfMetadata struct from the provided ELF file.
    pub fn from_elf_file(elf_path: &PathBuf) -> Result<Self, Error> {
        let file_contents = fs::read(elf_path)?;
        let elf_file = ElfFile::parse(&file_contents[..])?;
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
                println!("Warning: Level {:?} not found in elf file", level);
            }
        }

        Ok(Self {
            timestamp_freq,
            strings: interned_strings.into(),
            log_sections: sections,
        })
    }

    fn recover_interned_string<'a>(
        &self,
        str_buffer: &'a [u8],
    ) -> Result<(&'a [u8], &'a [u8], &'a [u8]), Error> {
        let end_of_string = str_buffer
            .iter()
            .position(|&c| c == b'\0')
            .ok_or(Error::InvalidFormatString)?;
        let mut str_buffer = &str_buffer[..end_of_string];

        let at_pos = str_buffer
            .iter()
            .position(|&c| c == b'@')
            .ok_or(Error::InvalidFormatString)?;
        let file_name = &str_buffer[..at_pos];
        str_buffer = &str_buffer[at_pos + 1..];

        let at_pos = str_buffer
            .iter()
            .position(|&c| c == b'@')
            .ok_or(Error::InvalidFormatString)?;
        let line_number = &str_buffer[..at_pos];
        let format = &str_buffer[at_pos + 1..];

        Ok((file_name, line_number, format))
    }

    fn format_string(&self, format: &[u8], arguments: &[u8]) -> Result<String, Error> {
        let mut formatted_str = String::new();
        let mut format = format;
        let mut arguments = arguments;
        while !format.is_empty() {
            match format[0] {
                b'%' => {
                    match format[1] {
                        b's' => {
                            let nul_range_end = arguments
                                .iter()
                                .position(|&c| c == b'\0')
                                .unwrap_or(arguments.len());

                            let res = std::str::from_utf8(&arguments[..nul_range_end]).unwrap();
                            formatted_str.push_str(res);
                            arguments = &arguments[nul_range_end + 1..];
                        }
                        b'd' => {
                            let integer_val = i32::from_le_bytes(
                                arguments.try_into().or(Err(Error::MissingLogArgument))?,
                            );
                            let str = format!("{}", integer_val);
                            formatted_str.push_str(&str);
                            arguments = &arguments[std::mem::size_of::<i32>()..];
                        }
                        b'u' => {
                            let integer_val = u32::from_le_bytes(
                                arguments.try_into().or(Err(Error::MissingLogArgument))?,
                            );
                            let str = format!("{}", integer_val);
                            formatted_str.push_str(&str);
                            arguments = &arguments[std::mem::size_of::<u32>()..];
                        }
                        _ => {
                            return Err(Error::InvalidFormatSpecifier(format[1] as char));
                        }
                    }
                    format = &format[2..];
                }
                _ => {
                    let percentage_or_end = format
                        .iter()
                        .position(|&c| c == b'%')
                        .unwrap_or(format.len());
                    let format_chunk = std::str::from_utf8(&format[..percentage_or_end]).unwrap();
                    formatted_str.push_str(format_chunk);
                    format = &format[percentage_or_end..];
                }
            }
        }

        Ok(formatted_str)
    }

    fn get_log_section(&self, str_ptr: u64) -> &LogSection {
        let log_section = self
            .log_sections
            .iter()
            .find(|&x| str_ptr >= x.start && str_ptr < x.end);

        match log_section {
            Some(section) => section,
            None => &LogSection {
                level: LogLevel::Unknown,
                start: 0u64,
                end: 0u64,
            },
        }
    }

    /// Parses a Postform message from the passed buffer.
    /// If the buffer is invalid it may return an error.
    pub fn parse(&self, mut message: &[u8]) -> Result<Log, Error> {
        let timestamp = u64::from_le_bytes(
            message[..std::mem::size_of::<u64>()]
                .try_into()
                .or(Err(Error::InvalidLogMessage))?,
        ) as f64
            / self.timestamp_freq;
        message = &message[std::mem::size_of::<u64>()..];

        let str_ptr = u32::from_le_bytes(
            message[..std::mem::size_of::<u32>()]
                .try_into()
                .or(Err(Error::InvalidLogMessage))?,
        ) as u64;
        message = &message[std::mem::size_of::<u32>()..];

        let mappings = &self.strings[str_ptr as usize..];
        let (file_name, line_number, format_str) = self.recover_interned_string(mappings)?;
        let formatted_str = self.format_string(format_str, message)?;
        let log_section = self.get_log_section(str_ptr);

        Ok(Log {
            timestamp,
            level: log_section.level,
            message: formatted_str,
            file_name: String::from_utf8_lossy(file_name).to_string(),
            line_number: String::from_utf8_lossy(line_number).to_string(),
        })
    }
}
