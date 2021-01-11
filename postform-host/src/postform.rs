use object::read::{File as ElfFile, Object, ObjectSection, ObjectSymbol};
use std::{convert::TryInto, fs, path::PathBuf};

#[derive(Copy, Clone, Debug)]
pub enum Error {
    FileNotFound,
    UnexpectedFileFormat,
    LevelNotFound,
}

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

pub struct ElfMetadata {
    timestamp_freq: f64,
    strings: Vec<u8>,
    log_sections: Vec<LogSection>,
}

impl ElfMetadata {
    pub fn parse_elf_file(elf_path: &PathBuf, timestamp_freq: f64) -> Result<Self, Error> {
        let file_contents = match fs::read(elf_path) {
            Ok(content) => content,
            Err(_) => {
                return Err(Error::FileNotFound);
            }
        };

        let elf_file = ElfFile::parse(&file_contents[..]).unwrap();
        let string_section = elf_file.section_by_name(".interned_strings").unwrap();
        let interned_strings = string_section.data().unwrap();

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
}

fn find_first_character_position(buffer: &[u8], char: u8) -> usize {
    buffer
        .iter()
        .position(|&c| c == char)
        .unwrap_or(buffer.len())
}

fn recover_format_string(mut str_buffer: &[u8]) -> (&[u8], &[u8], &[u8]) {
    let at_pos = find_first_character_position(str_buffer, b'@');
    let file_name = &str_buffer[..at_pos];
    str_buffer = &str_buffer[at_pos + 1..];

    let at_pos = find_first_character_position(str_buffer, b'@');
    let line_number = &str_buffer[..at_pos];
    str_buffer = &str_buffer[at_pos + 1..];

    let nul_char_pos = find_first_character_position(str_buffer, b'\0');
    let format = &str_buffer[..nul_char_pos];

    (file_name, line_number, format)
}

fn format_string(format: &[u8], arguments: &[u8]) -> String {
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
                            arguments.try_into().expect("Not enough data for argument"),
                        );
                        let str = format!("{}", integer_val);
                        formatted_str.push_str(&str);
                        arguments = &arguments[std::mem::size_of::<i32>()..];
                    }
                    b'u' => {
                        let integer_val = u32::from_le_bytes(
                            arguments.try_into().expect("Not enough data for argument"),
                        );
                        let str = format!("{}", integer_val);
                        formatted_str.push_str(&str);
                        arguments = &arguments[std::mem::size_of::<u32>()..];
                    }
                    _ => {
                        let format_spec = std::str::from_utf8(&format[..2]).unwrap();
                        panic!("Unknown format specifier: {}", format_spec);
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

    formatted_str
}

fn get_log_section(interned_string_info: &ElfMetadata, str_ptr: u64) -> &LogSection {
    let log_section = interned_string_info
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

pub struct Log {
    pub timestamp: f64,
    pub level: LogLevel,
    pub message: String,
    pub file_name: String,
    pub line_number: String,
}

pub fn parse_received_message(interned_string_info: &ElfMetadata, mut message: &[u8]) -> Log {
    let timestamp = u64::from_le_bytes(
        message[..std::mem::size_of::<u64>()]
            .try_into()
            .expect("Not enough data received for timestamp"),
    ) as f64
        / interned_string_info.timestamp_freq;
    message = &message[std::mem::size_of::<u64>()..];

    let str_ptr = u32::from_le_bytes(
        message[..std::mem::size_of::<u32>()]
            .try_into()
            .expect("Not enough data received for format string pointer"),
    ) as u64;
    message = &message[std::mem::size_of::<u32>()..];

    let mappings = &interned_string_info.strings[str_ptr as usize..];
    let (file_name, line_number, format) = recover_format_string(mappings);
    let formatted_str = format_string(format, message);
    let log_section = get_log_section(interned_string_info, str_ptr);

    Log {
        timestamp,
        level: log_section.level,
        message: formatted_str,
        file_name: String::from_utf8_lossy(file_name).to_string(),
        line_number: String::from_utf8_lossy(line_number).to_string(),
    }
}
