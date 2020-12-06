use cobs::CobsDecoder;
use object::read::{File as ElfFile, Object, ObjectSection, ObjectSymbol};
use probe_rs::config::registry;
use probe_rs::{Probe};
use probe_rs_rtt::Rtt;
use std::fs;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use structopt::StructOpt;
use termion::color;
use deferred_logger::{download_firmware, run_core};

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


struct LogSection {
    name: &'static str,
    color: color::Rgb,
    start: u32,
    end: u32,
}

struct InternedStringInfo {
    strings: Vec<u8>,
    log_sections: Vec<LogSection>,
}

fn parse_log_section<'a, T: Object<'a, 'a>>(
    elf_file: &'a T,
    section: &'static str,
    color: color::Rgb,
) -> LogSection {
    let start_symbol_name = format!("__Interned{}Start", section);
    let end_symbol_name = format!("__Interned{}End", section);
    let start = elf_file
        .symbols()
        .find(|x| x.name().unwrap() == start_symbol_name)
        .expect(&format!("Error finding symbol {}", start_symbol_name))
        .address() as u32;

    let end = elf_file
        .symbols()
        .find(|x| x.name().unwrap() == end_symbol_name)
        .expect(&format!("Error finding symbol {}", end_symbol_name))
        .address() as u32;

    LogSection {
        name: section,
        color: color,
        start: start,
        end: end,
    }
}

fn parse_elf_file(elf_path: &PathBuf) -> InternedStringInfo {
    let file_contents = fs::read(elf_path).unwrap();
    let elf_file = ElfFile::parse(&file_contents[..]).unwrap();
    let string_section = elf_file.section_by_name(".interned_strings").unwrap();
    let interned_strings = string_section.data().unwrap();

    let mut sections: Vec<LogSection> = vec![];
    sections.push(parse_log_section(
        &elf_file,
        "Debug",
        color::Rgb(0u8, 255u8, 0u8),
    ));
    sections.push(parse_log_section(
        &elf_file,
        "Info",
        color::Rgb(255u8, 255u8, 0u8),
    ));
    sections.push(parse_log_section(
        &elf_file,
        "Warning",
        color::Rgb(255u8, 0xA5u8, 0u8),
    ));
    sections.push(parse_log_section(
        &elf_file,
        "Error",
        color::Rgb(255u8, 0u8, 0u8),
    ));

    InternedStringInfo {
        strings: interned_strings.into(),
        log_sections: sections,
    }
}

fn find_first_character_position<'a>(buffer: &'a [u8], char: u8) -> usize {
    buffer
        .iter()
        .position(|&c| c == char)
        .unwrap_or(buffer.len())
}

fn recover_format_string<'a>(mut str_buffer: &'a [u8]) -> (&'a [u8], &'a [u8], &'a [u8]) {
    let at_pos = find_first_character_position(str_buffer, b'@');
    let file_name = &str_buffer[..at_pos];
    str_buffer = &str_buffer[at_pos+1..];

    let at_pos = find_first_character_position(str_buffer, b'@');
    let line_number = &str_buffer[..at_pos];
    str_buffer = &str_buffer[at_pos+1..];

    let nul_char_pos = find_first_character_position(str_buffer, b'\0');
    let format = &str_buffer[..nul_char_pos];

    (file_name, line_number, format)
}

fn format_string(format: &[u8], arguments: &[u8]) -> String {
    let mut formatted_str = String::new();
    let mut format = format;
    let mut arguments = arguments;
    while format.len() != 0 {
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
                        let integer_val = (arguments[0] as i32) << 0
                            | (arguments[1] as i32) << 8
                            | (arguments[2] as i32) << 16
                            | (arguments[3] as i32) << 24;
                        let str = format!("{}", integer_val);
                        formatted_str.push_str(&str);
                        arguments = &arguments[4..];
                    }
                    b'u' => {
                        let integer_val = (arguments[0] as u32) << 0
                            | (arguments[1] as u32) << 8
                            | (arguments[2] as u32) << 16
                            | (arguments[3] as u32) << 24;
                        let str = format!("{}", integer_val);
                        formatted_str.push_str(&str);
                        arguments = &arguments[4..];
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

fn get_log_section<'a>(
    interned_string_info: &'a InternedStringInfo,
    str_ptr: u32,
) -> &'a LogSection {
    let log_section = interned_string_info.log_sections.iter().find(|&x| {
        return str_ptr >= x.start && str_ptr < x.end;
    });

    match log_section {
        Some(section) => {
            return section;
        }
        None => {
            return &LogSection {
                name: "Unknown",
                color: color::Rgb(255u8, 0u8, 0u8),
                start: 0u32,
                end: 0u32,
            };
        }
    }
}

fn parse_received_message(interned_string_info: &InternedStringInfo, message: &[u8]) {
    let timestamp = (message[0] as u64) << 0
        | (message[1] as u64) << 8
        | (message[2] as u64) << 16
        | (message[3] as u64) << 24
        | (message[4] as u64) << 32
        | (message[5] as u64) << 40
        | (message[6] as u64) << 48
        | (message[7] as u64) << 56;
    let str_ptr = (message[8] as u32) << 0
        | (message[9] as u32) << 8
        | (message[10] as u32) << 16
        | (message[11] as u32) << 24;
    let mappings = &interned_string_info.strings[str_ptr as usize..];
    let (file_name, line_number, format) = recover_format_string(mappings);
    let formatted_str = format_string(format, &message[12..]);
    let log_section = get_log_section(interned_string_info, str_ptr);
    println!(
        "{timestamp:<11}{color}{level:<11}{reset_color}: {msg}",
        timestamp=timestamp,
        color=color::Fg(log_section.color),
        level=log_section.name,
        reset_color=color::Fg(color::Reset),
        msg=formatted_str
    );
    println!(
        "{color}└── File: {file_name}, Line number: {line_number}{reset}",
        color=color::Fg(color::LightBlack),
        file_name=String::from_utf8_lossy(file_name),
        line_number=String::from_utf8_lossy(line_number),
        reset=color::Fg(color::Reset)
    );
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

    let elf_name = opts.elf.unwrap();
    let interned_string_info = parse_elf_file(&elf_name);

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
                for i in 0..count {
                    match decoder.feed(buf[i]) {
                        Ok(Some(msg_len)) => {
                            drop(decoder);
                            parse_received_message(&interned_string_info, &dec_buf[..msg_len]);
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
