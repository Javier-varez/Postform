use object::read::{File as ElfFile, Object, ObjectSymbol};
use postform_decoder::{print_log, ElfMetadata, SerialDecoder, POSTFORM_VERSION};
use postform_rtt::{
    attach_rtt, configure_rtt_mode, disable_cdebugen, download_firmware, run_core, RttError,
    RttMode,
};
use probe_rs::{DebugProbeError, DebugProbeSelector, Probe};
use std::sync::atomic::{AtomicBool, Ordering};
use std::{
    fs,
    path::PathBuf,
    sync::{Arc, Mutex},
};
use structopt::StructOpt;
use thiserror::Error;

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
    let registry = probe_rs::config::families().expect("Could not retrieve chip family registry");
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

#[derive(Error, Debug)]
enum ProbeErrors {
    #[error("There are no probes available")]
    NoProbesAvailable,
    #[error(
        "More than one probe available. Select one of them with --probe-selector or --probe-index"
    )]
    MoreThanOneProbeAvailable,
    #[error("Probe index is larger than the number of probes available")]
    IndexOutOfRange,
    #[error("Specific probe error from probe-rs")]
    OpenError(#[from] DebugProbeError),
}

/// Opens a probe with the given index or the first one if there is only one
fn open_probe(index: Option<usize>) -> Result<Probe, ProbeErrors> {
    let probes = Probe::list_all();
    if probes.is_empty() {
        return Err(ProbeErrors::NoProbesAvailable);
    }

    let index = match index {
        Some(index) if (index >= probes.len()) => {
            return Err(ProbeErrors::IndexOutOfRange);
        }
        None if (probes.len() != 1) => {
            return Err(ProbeErrors::MoreThanOneProbeAvailable);
        }
        Some(index) => index,
        None => 0,
    };

    Ok(Probe::open(probes[index].clone())?)
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
    #[structopt(long, required_unless_one(&["list-chips", "list-probes", "version"]), env = "POSTFORM_CHIP")]
    chip: Option<String>,

    /// The probe to open. The format is <VID>:<PID>[:<SERIAL>].
    #[structopt(long, env = "POSTFORM_PROBE")]
    probe_selector: Option<DebugProbeSelector>,

    /// Index of the probe to open. Can be obtained with --list-probes.
    #[structopt(long = "probe-index")]
    probe_index: Option<usize>,

    /// Path to an ELF firmware file.
    #[structopt(name = "ELF", parse(from_os_str), required_unless_one(&["list-chips", "list-probes", "version"]))]
    elf: Option<PathBuf>,

    /// Attaches to a running target instead of downloading the firmware.
    #[structopt(long, short)]
    attach: bool,

    /// Disables FW version check.
    #[structopt(long, short = "d")]
    disable_version_check: bool,

    #[structopt(long, short = "V")]
    version: bool,

    #[structopt(long, short)]
    gdb_server: bool,

    #[structopt(long, short)]
    channel: Option<usize>,
}

fn main() -> color_eyre::eyre::Result<()> {
    color_eyre::install()?;
    env_logger::init();

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
    let elf_metadata = ElfMetadata::from_elf_file(&elf_name, opts.disable_version_check)?;

    let probe = if let Some(probe_name) = opts.probe_selector {
        Probe::open(probe_name)?
    } else {
        open_probe(opts.probe_index)?
    };

    let rtt_channel = opts.channel.unwrap_or(0);

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
                is_app_running.store(false, Ordering::Relaxed);
            })?;
        }
        if !opts.attach {
            download_firmware(&session, &elf_name)?;
        }

        configure_rtt_mode(
            session.clone(),
            segger_rtt_addr,
            rtt_channel,
            RttMode::Blocking,
        )?;
        let mut rtt = attach_rtt(session.clone(), &elf_file)?;
        run_core(session.clone())?;

        if !opts.gdb_server {
            disable_cdebugen(session.clone())?;
        } else {
            let session = session.clone();
            let _ = Some(std::thread::spawn(move || {
                let gdb_connection_string = "127.0.0.1:1337";
                // This next unwrap will always resolve as the connection string is always Some(T).
                log::info!("Firing up GDB stub at {}.", gdb_connection_string);
                if let Err(e) = probe_rs_gdb_server::run(Some(gdb_connection_string), &session) {
                    log::error!("During the execution of GDB an error was encountered:");
                    log::error!("{:?}", e);
                }
            }));
        }

        if let Some(log_channel) = rtt.up_channels().take(rtt_channel) {
            let mut buffer = [0u8; 1024];
            let mut decoder = SerialDecoder::new(&elf_metadata);
            loop {
                let count = log_channel.read(&mut buffer[..])?;
                if count > 0 {
                    decoder.feed_and_do(&buffer[..count], |log| {
                        print_log(&log);
                    });
                }

                // Close application if requested
                if !is_app_running.load(Ordering::Relaxed) {
                    log::info!("Closing application");
                    break;
                }
            }
        }
        configure_rtt_mode(session, segger_rtt_addr, rtt_channel, RttMode::NonBlocking)?;
    }
    Ok(())
}
