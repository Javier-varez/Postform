use color_eyre::eyre::Result;
use colored::Colorize;
use postform_decoder::{print_log, Decoder, ElfMetadata, POSTFORM_VERSION};
use std::convert::TryInto;
use std::io::prelude::*;
use std::{fs, path::PathBuf};
use structopt::StructOpt;

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

    /// Disables FW version check.
    #[structopt(long, short = "d")]
    disable_version_check: bool,

    #[structopt(long, short = "V")]
    version: bool,
}

fn main() -> Result<()> {
    color_eyre::install()?;
    env_logger::init();

    let opts = Opts::from_args();

    if opts.version {
        print_version();
        return Ok(());
    }

    let elf_name = opts.elf.unwrap();
    let elf_metadata = ElfMetadata::from_elf_file(&elf_name, opts.disable_version_check)?;

    let mut log_file = fs::File::open(opts.log_file.unwrap())?;
    let mut log_data = vec![];
    log_file.read_to_end(&mut log_data)?;

    let mut log_data = &log_data[..];
    let mut decoder = Decoder::new(&elf_metadata);
    loop {
        let (size_bits, rest) = log_data.split_at(std::mem::size_of::<u32>());
        let size = u32::from_le_bytes(size_bits.try_into().unwrap()) as usize;
        match decoder.decode(&rest[..size]) {
            Ok(log) => print_log(&log),
            Err(error) => {
                println!("{}{}", "Error parsing log: ".red(), error);
            }
        };

        log_data = &log_data[4 + size..];
        if log_data.is_empty() {
            break;
        }
    }

    Ok(())
}
