use prettydiff::text::diff_lines;
use std::path::Path;
use structopt::StructOpt;
use xshell::{cmd, cp, cwd, mkdir_p, pushd, read_file, rm_rf, write_file, Result};

#[derive(Debug, StructOpt)]
enum Core {
    /// ARM Cortex-M0, uses armv6m toolchain
    CortexM0,
    /// ARM Cortex-M3, uses armv7m toolchain
    CortexM3,
}

#[derive(Debug, StructOpt)]
#[structopt(name = "xtask", about = "runs automated tasks with cargo")]
enum Options {
    /// Builds all postform host binaries and libraries
    Build {
        /// Builds the release version of the host tools
        #[structopt(long)]
        release: bool,
    },
    /// Builds and example firmware application for a Cortex-M3 MCU
    BuildFirmware {
        #[structopt(long)]
        force: bool,
        #[structopt(subcommand)]
        core: Option<Core>,
        #[structopt(long)]
        compdb: bool,
        #[structopt(long)]
        dry_run: bool,
    },
    /// Bless the test output given by the current implementation of Postform
    Bless,
    /// Run Postform tests and display an output diff
    Test,
    /// Runs multiple linters on the code
    Lint,
    /// Cleans all target folders
    Clean,
    /// Runs the example application on an STM32F103C8 microcontroller
    RunExampleApp,
}

fn run_in_docker<T>(args: T)
where
    T: std::iter::Iterator<Item = String>,
{
    // rerun in docker
    let root_dir = cwd().unwrap();
    let uid = cmd!("id -u").read().unwrap();
    let gid = cmd!("id -g").read().unwrap();

    cmd!("docker run --rm -v {root_dir}:{root_dir} --workdir {root_dir} --user {uid}:{gid} javiervarez/ate_builder:main cargo xtask").args(args).run().unwrap();
}

fn main() {
    // Check if we should restart under docker
    let mut args = std::env::args();
    let first_arg = args.nth(1);
    if first_arg.unwrap_or_default() == "docker" {
        run_in_docker(args);
        return;
    }

    let opts = Options::from_args();

    match opts {
        Options::Build { release } => build(release),
        Options::BuildFirmware {
            core,
            force,
            compdb,
            dry_run,
        } => build_firmware(core, force, compdb, dry_run),
        Options::Bless => bless_cxx_tests(),
        Options::Test => run_tests(),
        Options::Lint => run_lint(),
        Options::Clean => clean(),
        Options::RunExampleApp => run_example_app(),
    }
}

fn build_cxx_tests(root_dir: &Path, build_dir: &Path) -> Result<()> {
    rm_rf(build_dir)?;
    mkdir_p(build_dir)?;
    let _dir = pushd(build_dir)?;

    cmd!("cmake -G Ninja -DPOSTFORM_BUILD_EXAMPLES=true -DPOSTFORM_BUILD_UNITTESTS=true -DCMAKE_CXX_COMPILER=clang++ {root_dir}")
        .run()?;
    cmd!("cmake --build .").run()?;
    Ok(())
}

fn build(release: bool) {
    let release = if release { Some("--release") } else { None };
    cmd!("cargo build").args(release).run().unwrap();
}

fn build_firmware(core: Option<Core>, force_clean: bool, compdb: bool, dry_run: bool) {
    let root_dir = cwd().unwrap();
    let mut build_dir = root_dir.clone();
    build_dir.push("fw_build");

    let core = core.unwrap_or(Core::CortexM3);

    match core {
        Core::CortexM0 => build_dir.push("m0"),
        Core::CortexM3 => build_dir.push("m3"),
    }

    let toolchain = match core {
        Core::CortexM0 => "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/armv6m.cmake",
        Core::CortexM3 => "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/armv7m.cmake",
    };

    if force_clean {
        rm_rf(&build_dir).unwrap();
    }
    mkdir_p(&build_dir).unwrap();
    let _dir = pushd(&build_dir).unwrap();

    let cmake_cmd = cmd!("cmake -G Ninja -DPOSTFORM_BUILD_EXAMPLES=true {toolchain} -DPOSTFORM_BUILD_TARGET_APP=true -DCMAKE_CXX_COMPILER=clang++");
    if compdb {
        cmake_cmd
            .arg("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
            .arg(root_dir.clone())
            .run()
            .unwrap();
        let mut compdb_src = build_dir;
        compdb_src.push("compile_commands.json");

        let mut compdb_dest = root_dir;
        compdb_dest.push("compile_commands.json");
        cp(compdb_src, compdb_dest).unwrap();
    } else {
        cmake_cmd.arg(root_dir).run().unwrap();
    }
    if !dry_run {
        cmd!("cmake --build .").run().unwrap();
    }
}

fn clean() {
    let root_dir = cwd().unwrap();
    let mut build_dir = root_dir.clone();
    build_dir.push("fw_build");
    let mut test_dir = root_dir;
    test_dir.push("cxx_build");

    rm_rf(build_dir).unwrap();
    rm_rf(test_dir).unwrap();
    cmd!("cargo clean").run().unwrap();
}

fn run_unit_tests(build_dir: &Path) -> Result<()> {
    let mut postform_test = build_dir.to_owned();
    postform_test.push("libpostform");
    postform_test.push("postform_tests");

    cmd!("{postform_test}").run()?;
    Ok(())
}

fn run_system_tests(root_dir: &Path, build_dir: &Path) -> Result<()> {
    let mut file_name = build_dir.to_owned();
    file_name.push("logs.txt");

    let mut postform_test = build_dir.to_owned();
    postform_test.push("app");
    postform_test.push("postform_test");

    cmd!("{postform_test} {file_name}").run()?;
    let text = cmd!("cargo run --bin=postform_persist -- {postform_test} {file_name}").read()?;

    let mut blessed_file = root_dir.to_owned();
    blessed_file.push("expected_log.txt");
    let blessed = read_file(&blessed_file)?;

    // Replace absolute paths by relative paths
    let text = text.replace(&root_dir.to_str().unwrap(), "..");

    let diff = diff_lines(&blessed, &text).set_diff_only(true);
    let mut diffs = diff.diff().into_iter().filter(|x| match x {
        prettydiff::basic::DiffOp::Insert(_) => true,
        prettydiff::basic::DiffOp::Replace(_, _) => true,
        prettydiff::basic::DiffOp::Remove(_) => true,
        prettydiff::basic::DiffOp::Equal(_) => false,
    });
    if let Some(_) = diffs.next() {
        println!("{}", diff.format());
        panic!("There are differences between the expected test output and the output of the executed binary. If these differences are legitimate or desired, please run `cargo xtask bless` to record them");
    } else {
        println!("Results match expected data, tests PASSED");
    }
    Ok(())
}

fn run_rust_tests() -> Result<()> {
    cmd!("cargo test").run()?;
    Ok(())
}

fn run_tests() {
    let root_dir = cwd().unwrap();
    let mut build_dir = root_dir.clone();
    build_dir.push("cxx_build");

    build_cxx_tests(&root_dir, &build_dir).unwrap();
    run_unit_tests(&build_dir).unwrap();
    run_system_tests(&root_dir, &build_dir).unwrap();
    run_rust_tests().unwrap();
}

fn bless_cxx_tests() {
    let root_dir = cwd().unwrap();
    let mut build_dir = root_dir.clone();
    build_dir.push("cxx_build");

    let mut file_name = build_dir.clone();
    file_name.push("logs.txt");

    build_cxx_tests(&root_dir, &build_dir).unwrap();

    let mut postform_test = build_dir.clone();
    postform_test.push("app");
    postform_test.push("postform_test");

    cmd!("{postform_test} {file_name}").run().unwrap();
    let text = cmd!("cargo run --bin=postform_persist -- {postform_test} {file_name}")
        .read()
        .unwrap();

    // Replace absolute paths by relative paths
    let text = text.replace(&root_dir.to_str().unwrap(), "..");

    let mut blessed_file = root_dir;
    blessed_file.push("expected_log.txt");
    write_file(&blessed_file, text).unwrap();
}

fn run_example_app() {
    // build the firmware, then run postform_rtt
    build_firmware(Some(Core::CortexM3), false, false, false);
    cmd!("cargo run --bin=postform_rtt -- --chip STM32F103C8 fw_build/m3/app/postform_format")
        .run()
        .unwrap();
}

fn run_lint() {
    cmd!("cargo clippy").run().unwrap();
    build_firmware(Some(Core::CortexM3), false, true, true);
    cmd!("run-clang-tidy-12").run().unwrap();
}
