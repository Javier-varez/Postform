use cobs::CobsDecoder;
use postform_host::{download_firmware, run_core, ElfMetadata, LogLevel};
use probe_rs::config::registry;
use probe_rs::Probe;
use probe_rs_rtt::Rtt;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use structopt::StructOpt;
use termion::color;

use color_eyre::eyre::Result;

fn print_probes() {
    let probes = Probe::list_all();

    if !probes.is_empty() {
        println!("The following devices were found:");
        probes
            .iter()
            .enumerate()
            .for_each(|(num, link)| println!("[{}]: {:?}", num, link));
    } else {
        println!("No devices were found.");
    }
}

fn print_chips() {
    let registry = registry::families().expect("Could not retrieve chip family registry");
    for chip_family in registry {
        println!("{}", chip_family.name);
        println!("    Variants:");
        for variant in chip_family.variants.iter() {
            println!("        {}", variant.name);
        }
    }
}

#[derive(Debug, StructOpt)]
#[structopt(name = "deferred-logger")]
struct Opts {
    /// List supported chips and exit.
    #[structopt(long)]
    list_chips: bool,

    /// Lists all the connected probes and exit.
    #[structopt(long)]
    list_probes: bool,

    /// The chip.
    #[structopt(long, required_unless_one(&["list-chips", "list-probes"]), env = "PROBE_RUN_CHIP")]
    chip: Option<String>,

    /// Path to an ELF firmware file.
    #[structopt(name = "ELF", parse(from_os_str), required_unless_one(&["list-chips", "list-probes"]))]
    elf: Option<PathBuf>,
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

    if opts.list_probes {
        print_probes();
        return Ok(());
    }

    if opts.list_chips {
        print_chips();
        return Ok(());
    }

    let elf_name = opts.elf.unwrap();
    let elf_metadata = ElfMetadata::from_elf_file(&elf_name)?;

    let probes = Probe::list_all();
    if probes.len() > 1 {
        println!("More than one probe conected! {:?}", probes);
        return Ok(());
    }
    let probe = probes[0].open().unwrap();

    if let Some(chip) = opts.chip {
        let session = Arc::new(Mutex::new(probe.attach(chip).unwrap()));
        download_firmware(&session, &elf_name);

        let mut rtt = Rtt::attach(session.clone()).unwrap();
        println!("Rtt connected");

        run_core(session);

        if let Some(log_channel) = rtt.up_channels().take(0) {
            let mut dec_buf = [0u8; 4096];
            let mut buf = [0u8; 4096];
            let mut decoder = CobsDecoder::new(&mut dec_buf);
            loop {
                let count = log_channel.read(&mut buf[..]).unwrap();
                for data_byte in buf.iter().take(count) {
                    match decoder.feed(*data_byte) {
                        Ok(Some(msg_len)) => {
                            drop(decoder);
                            handle_log(&elf_metadata, &dec_buf[..msg_len]);
                            decoder = CobsDecoder::new(&mut dec_buf[..]);
                        }
                        Err(decoded_len) => {
                            drop(decoder);
                            println!("Cobs decoding failed after {} bytes", decoded_len);
                            println!("Decoded buffer: {:?}", &dec_buf[..decoded_len]);
                            return Ok(());
                        }
                        Ok(None) => {}
                    }
                }
            }
        }
    }
    Ok(())
}
