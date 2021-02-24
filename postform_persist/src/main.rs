use color_eyre::eyre::Result;
use postform_decoder::{ElfMetadata, LogLevel, POSTFORM_VERSION};
use std::convert::TryInto;
use std::io::prelude::*;
use std::{fs, path::PathBuf};
use structopt::StructOpt;
use termion::color;

fn print_version() {
    // version from Cargo.toml e.g. "0.1.4"
    println!("{} {}", env!("CARGO_PKG_NAME"), env!("CARGO_PKG_VERSION"));
    println!("supported Postform version: {}", POSTFORM_VERSION);
}

#[derive(Debug, StructOpt)]
#[structopt()]
struct Opts {
    /// Path to an ELF firmware file.
    #[structopt(name = "ELF", parse(from_os_str), required_unless_one(&["version"]))]
    elf: Option<PathBuf>,

    /// Path to the binary log file.
    #[structopt(name = "LOG_FILE", parse(from_os_str), required_unless_one(&["version"]))]
    log_file: Option<PathBuf>,

    #[structopt(long, short = "V")]
    version: bool,
}

fn color_for_level(level: LogLevel) -> String {
    match level {
        LogLevel::Debug => String::from(color::Green.fg_str()),
        LogLevel::Info => String::from(color::Yellow.fg_str()),
        LogLevel::Warning => color::Rgb(255u8, 0xA5u8, 0u8).fg_string(),
        LogLevel::Error => String::from(color::Red.fg_str()),
        LogLevel::Unknown => color::Rgb(255u8, 0u8, 0u8).fg_string(),
    }
}

fn handle_log(elf_metadata: &ElfMetadata, buffer: &[u8]) {
    match elf_metadata.parse(buffer) {
        Ok(log) => {
            println!(
                "{timestamp:<12.6} {color}{level:<11}{reset_color}: {msg}",
                timestamp = log.timestamp,
                color = color_for_level(log.level),
                level = log.level.to_string(),
                reset_color = color::Fg(color::Reset),
                msg = log.message
            );
            println!(
                "{color}└── File: {file_name}, Line number: {line_number}{reset}",
                color = color::Fg(color::LightBlack),
                file_name = log.file_name,
                line_number = log.line_number,
                reset = color::Fg(color::Reset)
            );
        }
        Err(error) => {
            println!(
                "{color}Error parsing log:{reset_color} {error}.",
                color = color::Fg(color::Red),
                error = error,
                reset_color = color::Fg(color::Reset)
            );
        }
    }
}

fn main() -> Result<()> {
    color_eyre::install()?;

    let opts = Opts::from_args();

    if opts.version {
        print_version();
        return Ok(());
    }

    let elf_name = opts.elf.unwrap();
    let elf_metadata = ElfMetadata::from_elf_file(&elf_name)?;

    let mut log_file = fs::File::open(opts.log_file.unwrap())?;
    let mut log_data = vec![];
    log_file.read_to_end(&mut log_data)?;

    let mut log_data = &log_data[..];
    loop {
        let (size_bits, rest) = log_data.split_at(std::mem::size_of::<u32>());
        let size = u32::from_le_bytes(size_bits.try_into().unwrap()) as usize;
        handle_log(&elf_metadata, &rest[..size]);
        log_data = &log_data[4 + size..];
        if log_data.is_empty() {
            break;
        }
    }

    Ok(())
}
