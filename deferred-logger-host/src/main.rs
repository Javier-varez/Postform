use object::read::{File as ElfFile, Object, ObjectSection};
use probe_rs::config::registry;
use probe_rs::flashing::{download_file, FileDownloadError, Format};
use probe_rs::{Probe, Session};
use probe_rs_rtt::Rtt;
use std::fs;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use std::time::Duration;
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

fn download_firmware(session: &Arc<Mutex<Session>>, elf: &PathBuf) {
    let mut mutex_guard = session.lock().unwrap();
    println!("Loading FW to target");
    match download_file(&mut mutex_guard, &elf, Format::Elf) {
        Err(error) => {
            match error {
                FileDownloadError::Elf(_) => {
                    println!("Error with elf file");
                }
                FileDownloadError::Flash(_) => {
                    println!("Error flashing the device");
                }
                _ => {
                    println!("Other error downloading FW");
                }
            }
            return;
        }
        _ => {}
    };

    let mut core = mutex_guard.core(0).unwrap();
    let _ = core.reset_and_halt(Duration::from_millis(10)).unwrap();
    core.run().unwrap();
    println!("Download complete!");
}

fn parse_elf_file(elf_path: &PathBuf) -> Vec<u8> {
    let file_contents = fs::read(elf_path).unwrap();
    let elf_file = ElfFile::parse(&file_contents[..]).unwrap();
    let string_section = elf_file.section_by_name(".interned_strings").unwrap();
    let data = string_section.data().unwrap();

    data.into()
}

fn recover_format_string<'a>(str_buffer: &'a [u8]) -> &'a str {
    let nul_range_end = str_buffer
        .iter()
        .position(|&c| c == b'\0')
        .unwrap_or(str_buffer.len());

    std::str::from_utf8(&str_buffer[..nul_range_end]).unwrap()
}

fn parse_received_message(mappings: &[u8], message: &[u8]) {
    let str_ptr = (message[0] as u32) << 0
        | (message[1] as u32) << 8
        | (message[2] as u32) << 16
        | (message[3] as u32) << 24;
    let string = recover_format_string(&mappings[str_ptr as usize..]);
    println!("String is: {}", string);
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

fn main() {
    let opts = Opts::from_args();

    if opts.list_probes {
        return print_probes();
    }

    if opts.list_chips {
        return print_chips();
    }

    println!("{:?}", opts);

    let elf_name = opts.elf.unwrap();
    let string_section = parse_elf_file(&elf_name);

    let probes = Probe::list_all();
    if probes.len() > 1 {
        println!("More than one probe conected! {:?}", probes);
        return;
    }
    let probe = probes[0].open().unwrap();

    if let Some(chip) = opts.chip {
        println!("Chip is {}", chip);
        let session = Arc::new(Mutex::new(probe.attach(chip).unwrap()));
        download_firmware(&session, &elf_name);

        let mut rtt = Rtt::attach(session.clone()).unwrap();
        println!("Rtt connected");
        if let Some(input) = rtt.up_channels().take(0) {
            loop {
                let mut buf = [0u8; 1024];
                let count = input.read(&mut buf[..]).unwrap();
                if count > 0 {
                    parse_received_message(&string_section[..], &buf[..count]);
                }
            }
        }
    }
}
