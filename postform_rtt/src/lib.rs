use color_eyre::eyre::Result;
use field_offset::offset_of;
use object::read::{File as ElfFile, Object, ObjectSymbol};
use probe_rs::{
    flashing::{download_file, Format},
    MemoryInterface, Session,
};
use probe_rs_rtt::{Rtt, ScanRegion};
use std::{
    fs,
    path::Path,
    sync::{Arc, Mutex},
    time::Duration,
};

/// RTT Errors for Postform Rtt
#[derive(Debug, thiserror::Error)]
pub enum RttError {
    #[error("Missing symbol {0}")]
    MissingSymbol(&'static str),
}

#[repr(C)]
struct RttHeader {
    id: [u8; 16],
    max_up_channels: u32,
    max_down_channels: u32,
}

#[repr(C)]
struct RttChannel {
    name: u32,
    buffer: u32,
    size: u32,
    write: u32,
    read: u32,
    flags: u32,
}

/// Downloads a FW ELF to the target in the associated session, halting the core at main.
pub fn download_firmware(session: &Arc<Mutex<Session>>, elf_path: &Path) -> Result<()> {
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
    let _ = core.reset_and_halt(Duration::from_millis(100))?;
    // If the main address has bit 0 set to indicate thumb mode in an ARM binary, let's set it back
    // to 0
    let main = (main.address() as u32) & !0x01;
    core.set_hw_breakpoint(main)?;
    log::debug!("Inserting breakpoint at main() @ 0x{:x}", main);
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
    channel_idx: usize,
    mode: RttMode,
) -> Result<()> {
    const CHANNEL_SIZE: usize = std::mem::size_of::<RttChannel>();
    // Header size is: 16 bytes for the rtt magic string
    //                 + 2 x uint32_t for number of up and down channels
    const HEADER_SIZE: usize = std::mem::size_of::<RttHeader>();
    let mut session_lock = session.lock().unwrap();
    let mut core = session_lock.core(0)?;
    let mode_flags_addr = rtt_addr as usize
        + HEADER_SIZE
        + CHANNEL_SIZE * channel_idx
        + offset_of!(RttChannel => flags).get_byte_offset();
    log::info!("Setting mode to {:?}", mode);
    core.write_word_32(mode_flags_addr as u32, mode as u32)?;

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
