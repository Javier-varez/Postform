# Postform

[![crates.io](https://meritbadge.herokuapp.com/postform_decoder)](https://crates.io/crates/postform_decoder) [![documentation](https://docs.rs/postform_decoder/badge.svg)](https://docs.rs/postform_decoder) ![Build Status](https://img.shields.io/github/workflow/status/javier-varez/deferred-logging/Target%20App)

`Postform` *(short for "Postponed formatting")* is a deferred-formatting logging system in C++ for 32 bit microcontrollers. This project is inspired/based on the [defmt](https://github.com/knurling-rs/defmt) rust crate from [Knurling-rs](https://knurling.ferrous-systems.com/). They are doing a splendid job that motivated me to create a C++ alternative following similar principles of operation, still leveraging rust for the host-side implementation of this logger.

## Table of Contents

- [About the Project](#about-the-project)
- [Project Status](#project-status)
- [Getting Started](#getting-started)
  - [Dependencies](#dependencies)
  - [Getting the Source](#getting-the-source)
  - [Building](#building)
  - [Usage](#usage)
- [Release Process](#release-process)
  - [Versioning](#versioning)
- [How to Get Help](#how-to-get-help)
- [Contributing](#contributing)
- [License](#license)
- [Authors](#authors)
- [Acknowledgments](#acknowledgments)

# About the Project

String formatting can be quite expensive in embedded devices. Logging is one of the main tools used by developers to debug and analyze the performance of their devices. To provide an efficient logging mechanism suitable for embedded devices, `Postform` doesn't perform on-device formatting. Instead it is postponed and performed by a host computer.

Postform serializes the raw data (format string + all arguments) and sends it to the host device over a given transport. The host is responsible of deserializing this data and perform the log formatting and displaying it for the user.

* The format string.
* All arguments needed for the format string.

On the other hand, there is no point in storing and transmitting the format, since these are known at compile-time. Generally speaking, it is more efficient to simply store an identifier to the format string and let the host find the corresponding string for the given ID. This reduces both the size of the binary and optimizes for runtime performance as usually transmitting an ID is faster than transmitting a large string.

Overall, postponed formatting offers some **benefits**:
* Smaller memory footprint, since format strings are not stored in the binary.
* Faster logging, since only the raw binary data is transmitted, which is typically smaller than the actual formatted string.
* Having no strings in the binary makes reverse engineering harder.

And, as you could expect, some **drawbacks**:
* There is some tooling required on the host side to read and interpret the logs.
* The metadata for the logs is stored separately, and they need to be in sync both on the FW side and host side.

**[Back to top](#table-of-contents)**

# Project Status

![Build Status](https://img.shields.io/github/workflow/status/javier-varez/deferred-logging/Target%20App)

`Postform` is under active development at this point in time. All contributions are really appreciated and encouraged. Feel free to open issues with suggestions for improvements, bugs that you've encountered, etc. If you'd like to contribute, please checkout the [contribution guidelines](docs/CONTRIBUTING.md).

Currently the default build targets an `Cortex-M` processor, however `Postform` aims to be a platform-independent logging library for 32-bit microcontrollers, we aim for portable code as much as possible. In fact, it is also built for the host for testing purposes, showing how portable the code actually is.

At this point the API is still volatile and might change in the future. `Postform` uses [Semantic Versioning](https://semver.org) for its releases, so you will know when a breaking API feature has been added when a major number changes. However, until we reach `1.0.0` we may still make breaking API/ABI changes even if the minor is incremented.

**[Back to top](#table-of-contents)**

# Getting Started

## Dependencies

`Postform` can be built and used with `clang` and `GCC`. The build is currently supported on `Ubuntu 20.04`.

Get the latest `GNU ARM Embedded toolchain` [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm). Make sure to add the bin folder to your path.

You can get `clang` using the apt repositories:

```bash
sudo apt install clang
```

`Postform` relies on a custom build system based on a non-recursive set of makefile templates. You can find a link to it [here](https://github.com/Javier-varez/buildsystem). The dependencies for the buildsystem are:

```bash
sudo apt install make
```

Alternatively, `CMake` can be used to build the `Postform` target library.

All the Rust code for the host has its dependencies indicated in the `Cargo.toml` file.

## Getting the Source

This project is [hosted on GitHub](https://github.com/Javier-varez/Postform). We use [Google Repo](https://gerrit.googlesource.com/git-repo/) to manage multiple repositories. You can get repo by running:

```bash
mkdir -p ~/.bin
PATH="${HOME}/.bin:${PATH}"
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/.bin/repo
chmod a+rx ~/.bin/repo
```

And then download the repository with:

```bash
mkdir postform_repo
cd postform_repo
repo init -u https://github.com/Javier-varez/manifest -g postform
repo sync -c -d -j$(nproc)
```

## Building

### Building the embedded C++ code

Building the C++ code is as simple as running make.
``` bash
make -j
```

This commands builds the `libpostform` library along with an example application for the target and an example application on the host for testing. The target will be available in `build/targets`. 

If instead you'd prefer to use `CMake` you can build the postform library with:

```bash
mkdir build
cd build
cmake ..
make -j
```

Keep in mind that the `CMake` build can be run with any compiler and for any target as usual with cmake. If you want to build it for the armv7m architecture you can use the following toolchain:

```bash
mkdir build
cd build
cmake -dcmake_toolchain_file=../cmake/toolchains/armv7m.cmake ..
make -j
```

Or to build it for the armv6m architecture:

```bash
mkdir build
cd build
cmake -dcmake_toolchain_file=../cmake/toolchains/armv6m.cmake ..
make -j
```
### Building the Rust code

Change directory to the `postform` directory and run:

```bash
cargo build
```

## Usage

In order to run the demo simply navigate to `postform-rtt` and run `cargo r`.

The example app is built for an `STM32F103C8` microcontrollers connected via a debugger compatible with `probe-rs`, like an `ST-Link` or a `J-Link`.

The postform-rtt binary will load the firmware into the `STM32F103C8` device and start listening for logs. You should see a similar output to:

```bash
$ cargo r
    Finished dev [unoptimized + debuginfo] target(s) in 0.02s
     Running `trtt/debug/postform_rtt --chip STM32F103C8 ../build/targets/format`
Loading FW to target
Download complete!
Rtt connected
0.000000     Info       : I am 28 years old...
└── File: app/src/main.cpp, Line number: 57
0.000019     Warning    : Third string! With multiple args and more numbers: -1124
└── File: app/src/main.cpp, Line number: 58
0.000044     Error      : Oh boy, error 234556 just happened
```

**[Back to top](#table-of-contents)**

# Release Process

## Versioning

This project uses [Semantic Versioning](http://semver.org/). Releases are tagged accordingly. Versions below `1.0.0` can have breaking API/ABI changes at any time.

**[Back to top](#table-of-contents)**

# How to Get Help

Feel free to reach out and open an issue if you need any help getting up and running with `Postform`.

**[Back to top](#table-of-contents)**

# Contributing

Public contributions are very welcome and appreciated. Please have a look at [CONTRIBUTING.md](docs/CONTRIBUTING.md) for details on the development process and kinds of contributions.

**[Back to top](#table-of-contents)**

# License

This project is licensed under the MIT License - see [LICENSE.md](LICENSE.md) file for details.

**[Back to top](#table-of-contents)**

# Authors

* **[Javier Alvarez](https://github.com/Javier-varez)** - *Initial work* - [AllThingsEmbedded](https://allthingsembedded.net/)

See also the list of [contributors](https://github.com/Javier-varez/Postform/graphs/contributors) who participated in this project.

**[Back to top](#table-of-contents)**

# Acknowledgments

I would like to sincerely thank the good people at [knurling-rs](https://github.com/knurling-rs) that inspired this work. This project is, after all, an alternative implementation of [defmt](https://github.com/knurling-rs/defmt) for usage with C++ code.

**[Back to top](#table-of-contents)**
