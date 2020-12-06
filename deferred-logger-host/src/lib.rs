use probe_rs::Session;
use std::sync::{Arc, Mutex};
use std::path::PathBuf;
use probe_rs::flashing::{download_file, FileDownloadError, Format};
use std::time::Duration;
use object::read::{File as ElfFile, Object, ObjectSymbol };
use std::fs;

/// Loads an ELF file at the given path to a running session
pub fn download_firmware(session: &Arc<Mutex<Session>>, elf_path: &PathBuf) {
    let mut mutex_guard = session.lock().unwrap();
    println!("Loading FW to target");
    match download_file(&mut mutex_guard, &elf_path, Format::Elf) {
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

    let file_contents = fs::read(elf_path).unwrap();
    let elf_file = ElfFile::parse(&file_contents[..]).unwrap();
    let main = elf_file.symbols().find(|s| s.name().unwrap() == "main").expect("main symbol not found!");

    let mut core = mutex_guard.core(0).unwrap();
    let _ = core.reset_and_halt(Duration::from_millis(10)).unwrap();
    core.set_hw_breakpoint(main.address() as u32).expect("Unable to set breakpoint in main");
    core.run().unwrap();
    core.wait_for_core_halted(Duration::from_secs(1)).expect("Didn't halt on main");
    println!("Download complete!");
}

pub fn run_core(session: Arc<Mutex<Session>>) {
    let mut mutex_guard = session.lock().unwrap();
    let mut core = mutex_guard.core(0).unwrap();
    core.clear_all_hw_breakpoints().unwrap();
    core.run().unwrap();
}
