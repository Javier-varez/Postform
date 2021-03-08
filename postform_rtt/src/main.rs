use cobs::CobsDecoder;
use color_eyre::eyre::Result;
use object::read::{File as ElfFile, Object, ObjectSymbol};
use postform_decoder::{ElfMetadata, POSTFORM_VERSION};
use postform_rtt::{
    attach_rtt, configure_rtt_mode, download_firmware, handle_log, run_core, RttError, RttMode,
};
use probe_rs::{config::registry, Probe};
use std::sync::atomic::{AtomicBool, Ordering};
use std::{
    fs,
    path::PathBuf,
    sync::{Arc, Mutex},
};
use structopt::StructOpt;

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

fn print_version() {
    // version from Cargo.toml e.g. "0.1.4"
    println!("{} {}", env!("CARGO_PKG_NAME"), env!("CARGO_PKG_VERSION"));
    println!("supported Postform version: {}", POSTFORM_VERSION);
}

#[derive(Debug, StructOpt)]
#[structopt()]
struct Opts {
    /// List supported chips and exit.
    #[structopt(long)]
    list_chips: bool,

    /// Lists all the connected probes and exit.
    #[structopt(long)]
    list_probes: bool,

    /// The chip.
    #[structopt(long, required_unless_one(&["list-chips", "list-probes", "version"]), env = "PROBE_RUN_CHIP")]
    chip: Option<String>,

    /// Path to an ELF firmware file.
    #[structopt(name = "ELF", parse(from_os_str), required_unless_one(&["list-chips", "list-probes", "version"]))]
    elf: Option<PathBuf>,

    #[structopt(long, short)]
    attach: bool,

    #[structopt(long, short = "V")]
    version: bool,
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

    if opts.version {
        print_version();
        return Ok(());
    }

    let elf_name = opts.elf.unwrap();
    let elf_metadata = ElfMetadata::from_elf_file(&elf_name)?;

    let probes = Probe::list_all();
    if probes.len() > 1 {
        println!("More than one probe conected! {:?}", probes);
        return Ok(());
    }
    let probe = probes[0].open()?;

    if let Some(chip) = opts.chip {
        let session = Arc::new(Mutex::new(probe.attach(chip)?));

        let elf_contents = fs::read(elf_name.clone())?;
        let elf_file = ElfFile::parse(&elf_contents)?;
        let segger_rtt = elf_file
            .symbols()
            .find(|s| s.name().unwrap() == "_SEGGER_RTT")
            .ok_or(RttError::MissingSymbol("_SEGGER_RTT"))?;
        let segger_rtt_addr = segger_rtt.address();
        let is_app_running = Arc::new(AtomicBool::new(true));

        {
            let is_app_running = is_app_running.clone();
            ctrlc::set_handler(move || {
                println!("Exiting application");
                is_app_running.store(false, Ordering::Relaxed);
            })?;
        }
        if !opts.attach {
            download_firmware(&session, &elf_name)?;
        }
        configure_rtt_mode(&session, segger_rtt_addr, RttMode::Blocking)?;

        let mut rtt = attach_rtt(session.clone(), &elf_file)?;
        run_core(session.clone())?;

        if let Some(log_channel) = rtt.up_channels().take(0) {
            let mut dec_buf = [0u8; 4096];
            let mut buf = [0u8; 4096];
            let mut decoder = CobsDecoder::new(&mut dec_buf);
            loop {
                let count = log_channel.read(&mut buf[..])?;
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
                            decoder = CobsDecoder::new(&mut dec_buf[..]);
                        }
                        Ok(None) => {}
                    }
                }
                if is_app_running.load(Ordering::Relaxed) {
                    break;
                }
            }
        }
        configure_rtt_mode(&session, segger_rtt_addr, RttMode::NonBlocking)?;
    }
    Ok(())
}
