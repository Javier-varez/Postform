use std::{
    env,
    error::Error,
    fs,
    path::{Path, PathBuf},
    process::Command,
};

fn main() -> Result<(), Box<dyn Error>> {
    let out = &PathBuf::from(env::var("OUT_DIR")?);
    let hash = Command::new("git")
        .args(&["describe"])
        .output()
        .ok()
        .and_then(|output| {
            if output.status.success() {
                String::from_utf8(output.stdout).ok()
            } else {
                None
            }
        });
    let version = if let Some(hash) = hash {
        hash
    } else {
        assert!(!Path::new(".git").exists(), "you need to install the `git` command line tool to install the git version of `probe-run`");

        // no git info -> assume crates.io
        std::env::var("CARGO_PKG_VERSION")?
    };

    fs::write(
        out.join("version.rs"),
        format!(
            r#"
/// Supported `postform` version
pub const POSTFORM_VERSION: &str = "{}";
"#,
            version.trim(),
        ),
    )?;

    // Now let's generate the C++ bindings for Postform
    let bindings = bindgen::Builder::default()
        .header("bindings/shared_types.hpp")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings for shared_types.hpp");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings.write_to_file(out_path.join("shared_types.rs"))?;
    Ok(())
}
