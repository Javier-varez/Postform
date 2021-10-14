use itertools::Itertools;
use prettydiff::text::diff_lines;
use std::path::Path;
use structopt::StructOpt;
use xshell::{cmd, cwd, mkdir_p, pushd, read_file, rm_rf, write_file, Result};

#[derive(Debug, StructOpt)]
#[structopt(name = "xtask", about = "runs automated tasks with cargo")]
enum Options {
    #[structopt(about = "Bless the test output given by the current implementation of Postform")]
    Bless,
    #[structopt(about = "Run Postform tests and display an output diff")]
    Test,
}

fn run_in_docker<T>(args: T)
where
    T: std::iter::Iterator<Item = String>,
{
    // rerun in docker
    let root_dir = cwd().unwrap();
    let xtask_invocation: String = Itertools::intersperse(args, " ".to_string()).collect();

    cmd!("docker run --rm -v {root_dir}:/workspace --workdir /workspace javiervarez/ate_builder:main cargo xtask {xtask_invocation}").run().unwrap();
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
        Options::Bless => bless_cxx_tests(),
        Options::Test => run_cxx_tests(),
    }
}

fn build_cxx_tests(root_dir: &Path, build_dir: &Path) -> Result<()> {
    rm_rf(build_dir)?;
    mkdir_p(build_dir)?;
    let _dir = pushd(build_dir)?;

    cmd!("cmake --version").run()?;
    cmd!("cmake -G Ninja -DPOSTFORM_BUILD_EXAMPLES=true -DCMAKE_CXX_COMPILER=clang++ {root_dir}")
        .run()?;
    cmd!("cmake --build .").run()?;
    Ok(())
}

fn run_cxx_tests() {
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

    let mut blessed_file = root_dir.clone();
    blessed_file.push("expected_log.txt");
    let blessed = read_file(&blessed_file).unwrap();

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
