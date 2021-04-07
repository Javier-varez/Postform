use color_eyre::eyre::Result;
use object::read::{File as ElfFile, Object, ObjectSymbol};
use postform_decoder::{Decoder, ElfMetadata, LogLevel};
use probe_rs::{
    flashing::{download_file, Format},
    MemoryInterface, Session,
};
use probe_rs_rtt::{Rtt, ScanRegion};
use std::{
    fs,
    path::PathBuf,
    sync::{Arc, Mutex},
    time::Duration,
};
use termion::color;

/// RTT Errors for Postform Rtt
#[derive(Debug, thiserror::Error)]
pub enum RttError {
    #[error("Missing symbol {0}")]
    MissingSymbol(&'static str),
}

/// Downloads a FW ELF to the target in the associated session, halting the core at main.
pub fn download_firmware(session: &Arc<Mutex<Session>>, elf_path: &PathBuf) -> Result<()> {
    let mut mutex_guard = session.lock().unwrap();
    log::info!("Loading FW to target");
    download_file(&mut mutex_guard, &elf_path, Format::Elf)?;
    log::info!("Download complete!");

    let file_contents = fs::read(elf_path)?;
    let elf_file = ElfFile::parse(&file_contents[..])?;
    let main = elf_file
        .symbols()
        .find(|s| s.name().unwrap() == "main")
        .ok_or(RttError::MissingSymbol("main"))?;

    let mut core = mutex_guard.core(0).unwrap();
    let _ = core.reset_and_halt(Duration::from_millis(10))?;
    core.set_hw_breakpoint(main.address() as u32)?;
    log::debug!("Inserting breakpoint at main() @ 0x{:x}", main.address());
    core.run()?;
    core.wait_for_core_halted(Duration::from_secs(1))?;
    log::debug!("Core halted at main()");

    Ok(())
}

/// Selects the execution mode for RTT in the target.
#[derive(Debug)]
pub enum RttMode {
    /// The target does not block when the buffer is full, overflowing it.
    NonBlocking = 1,
    /// The target blocks until the buffer is ready to receive more data.
    Blocking = 2,
}

/// Configures the selected RTT mode in the RTT control block at the given address.
pub fn configure_rtt_mode(
    session: Arc<Mutex<Session>>,
    rtt_addr: u64,
    mode: RttMode,
) -> Result<()> {
    let mut session_lock = session.lock().unwrap();
    let mut core = session_lock.core(0)?;
    let mode_flags_addr = rtt_addr as u32 + 44u32;
    log::info!("Setting mode to {:?}", mode);
    core.write_word_32(mode_flags_addr, mode as u32)?;

    Ok(())
}

/// Runs the core and clears all breakpoints
pub fn run_core(session: Arc<Mutex<Session>>) -> Result<()> {
    let mut session_lock = session.lock().unwrap();
    let mut core = session_lock.core(0)?;
    log::info!("Clearing breakpoints and entering run state");
    core.clear_all_hw_breakpoints()?;
    core.run()?;

    Ok(())
}

/// Disables C_DEBUGEN for a cortex-m mcu
pub fn disable_cdebugen(session: Arc<Mutex<Session>>) -> Result<()> {
    let mut session_lock = session.lock().unwrap();
    let mut core = session_lock.core(0)?;

    // We write the DHCSR register here in order to disable C_DEBUGEN.
    // Debugging the core is not possible while Postform is running, so at
    // least we should be able to use the DebugMonitor Exception.
    // We need to write the password to the DHCSR register in order for the
    // write to be successful
    let dhcsr_addr = 0xE000EDF0;
    let dhcsr_val = [0xA05F << 16];
    core.write_32(dhcsr_addr, &dhcsr_val)?;
    log::info!("Disabled debugging");
    Ok(())
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

/// Decodes a log from the buffer and prints it to stdout.
pub fn handle_log(elf_metadata: &ElfMetadata, buffer: &[u8]) {
    let mut decoder = Decoder::new(&elf_metadata);
    match decoder.decode(buffer) {
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

/// Attaches to RTT at the address of the `_SEGGER_RTT` symbol
pub fn attach_rtt(session: Arc<Mutex<Session>>, elf_file: &ElfFile) -> Result<Rtt> {
    let segger_rtt = elf_file
        .symbols()
        .find(|s| s.name().unwrap() == "_SEGGER_RTT")
        .ok_or(RttError::MissingSymbol("_SEGGER_RTT"))?;
    log::info!("Attaching RTT to address 0x{:x}", segger_rtt.address());
    let scan_region = ScanRegion::Exact(segger_rtt.address() as u32);
    Ok(Rtt::attach_region(session, &scan_region)?)
}
