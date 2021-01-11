use cobs::CobsDecoder;
use postform_host::{download_firmware, parse_received_message, run_core, ElfMetadata};
use probe_rs::config::registry;
use probe_rs::Probe;
use probe_rs_rtt::Rtt;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use structopt::StructOpt;
use termion::color;

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

    #[structopt(long, required_unless_one(&["list-chips", "list-probes"]))]
    timestamp_freq: Option<f64>,
}

fn handle_log(elf_metadata: &ElfMetadata, buffer: &[u8]) {
    let log = parse_received_message(elf_metadata, buffer);
    println!(
        "{timestamp:<12.6} {level:<11}: {msg}",
        timestamp = log.timestamp,
        level = log.level.to_string(),
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

fn main() {
    let opts = Opts::from_args();

    if opts.list_probes {
        return print_probes();
    }

    if opts.list_chips {
        return print_chips();
    }

    let elf_name = opts.elf.unwrap();
    let timestamp_freq = opts.timestamp_freq.unwrap();
    let elf_metadata = ElfMetadata::parse_elf_file(&elf_name, timestamp_freq).unwrap();

    let probes = Probe::list_all();
    if probes.len() > 1 {
        println!("More than one probe conected! {:?}", probes);
        return;
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
                            return;
                        }
                        Ok(None) => {}
                    }
                }
            }
        }
    }
}
