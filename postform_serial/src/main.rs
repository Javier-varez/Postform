use postform_decoder::{print_log, ElfMetadata, SerialDecoder, POSTFORM_VERSION};
use serialport::{self, FlowControl, Parity, StopBits};
use std::path::PathBuf;
use structopt::StructOpt;

/// Serial errors for Postform serial
#[derive(Debug, thiserror::Error)]
pub enum SerialError {
    #[error("Invalid parity requested \"{0}\"")]
    InvalidParityString(String),
    #[error("Invalid stop bits requested \"{0}\"")]
    InvalidStopBitsString(String),
}

fn print_version() {
    // version from Cargo.toml e.g. "0.1.4"
    println!("{} {}", env!("CARGO_PKG_NAME"), env!("CARGO_PKG_VERSION"));
    println!("supported Postform version: {}", POSTFORM_VERSION);
}

fn try_to_serial_parity(parity: &str) -> Result<Parity, SerialError> {
    match parity {
        "odd" => Ok(Parity::Odd),
        "even" => Ok(Parity::Even),
        "none" => Ok(Parity::None),
        _ => Err(SerialError::InvalidParityString(parity.to_owned())),
    }
}

fn try_to_serial_stop_bits(stop_bits: &str) -> Result<StopBits, SerialError> {
    match stop_bits {
        "1" => Ok(StopBits::One),
        "2" => Ok(StopBits::Two),
        _ => Err(SerialError::InvalidStopBitsString(stop_bits.to_owned())),
    }
}

#[derive(Debug, StructOpt)]
#[structopt()]
struct Opts {
    /// Path to an ELF firmware file.
    #[structopt(name = "ELF", parse(from_os_str), required_unless_one(&["version", "list-ports"]))]
    elf: Option<PathBuf>,

    /// Path to the binary log file.
    #[structopt(name = "port", required_unless_one(&["version", "list-ports"]))]
    port: Option<String>,

    /// Serial port baudrate. Defaults to 115200.
    #[structopt(long, short = "s")]
    baudrate: Option<u32>,

    /// Serial port stop bits number. Defaults to 1.
    #[structopt(long, parse(try_from_str=try_to_serial_stop_bits))]
    stop_bits: Option<StopBits>,

    /// Serial port parity configuration. Defaults to None.
    #[structopt(long, parse(try_from_str=try_to_serial_parity))]
    parity: Option<Parity>,

    /// Disables FW version check.
    #[structopt(long, short = "d")]
    disable_version_check: bool,

    /// Lists the available serial ports and exits.
    #[structopt(long)]
    list_ports: bool,

    /// Shows the version information.
    #[structopt(long, short = "V")]
    version: bool,
}

fn main() -> color_eyre::eyre::Result<()> {
    color_eyre::install()?;
    env_logger::init();

    let opts = Opts::from_args();

    if opts.version {
        print_version();
        return Ok(());
    }

    if opts.list_ports {
        let available_ports = serialport::available_ports()?;
        for port in available_ports {
            println!("{:?}", port);
        }
        return Ok(());
    }

    let elf_name = opts.elf.unwrap();
    let elf_metadata = ElfMetadata::from_elf_file(&elf_name, opts.disable_version_check)?;
    let mut decoder = SerialDecoder::new(&elf_metadata);

    let mut port = serialport::new(opts.port.unwrap(), opts.baudrate.unwrap_or(115200u32))
        .parity(opts.parity.unwrap_or(Parity::None))
        .stop_bits(opts.stop_bits.unwrap_or(StopBits::One))
        .flow_control(FlowControl::None)
        .open()?;
    port.set_timeout(std::time::Duration::from_millis(100))?;

    loop {
        let mut buffer = [0; 1024];
        let count = match port.read(&mut buffer[..]) {
            Ok(count) => Ok(count),
            Err(error) if error.kind() == std::io::ErrorKind::TimedOut => Ok(0),
            Err(error) => Err(error),
        }?;

        if count > 0 {
            decoder.feed_and_do(&buffer[..count], |log| {
                print_log(&log);
            });
        }
    }
}
