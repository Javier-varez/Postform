# Postform

[![crates.io](https://img.shields.io/crates/v/postform_decoder)](https://crates.io/crates/postform_decoder) [![documentation](https://docs.rs/postform_decoder/badge.svg)](https://docs.rs/postform_decoder) ![Build Status](https://img.shields.io/github/workflow/status/javier-varez/deferred-logging/Target%20App)

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

## Components

`Postform` is composed of the following components:
  * `libpostform`, which is a C++ library that can be linked against in embedded code. It handles the formatting and serialization of messages, providing some example transport layers for the log data.
  * `postform_decoder`, which is a Rust library that can parse and format log messages. Some other Rust binaries use this library to provide convenient usage and log parsing depending on the transport and format.
  * `postform_rtt`, which is a Rust binary that connects through RTT to the target using a debugger connection and reads the logs in runtime through the RTT transport, printing them to the console.
  * `postform_serial`, which is a Rust binary that uses a TTY device instead of RTT as a transport and displays log messages on the console.
  * `postform_persist`, which is a Rust binary that reads the log data generated by `libpostform` from a file and prints the messages to the console.

## Dependencies

`libpostform` can be built and used with `clang` and `GCC`. The build is currently supported on `Ubuntu 20.04` and the default toolchain configuration uses `clang` as a compiler and the `newlib` library from the `GCC ARM Embedded toolchain`. `libpostform` is tested with `clang-12`, but it is known to work with `clang-10` as well.

Get the latest `GNU ARM Embedded toolchain` [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm). Make sure to add the bin folder to your path.
The build system requires both `CMake`, `Ninja` and `Cargo` and uses [Cargo xtasks](https://github.com/matklad/cargo-xtask) to automate and script the build process.

To install the dependencies from the Ubuntu apt repositories run:

```bash
sudo apt install cmake ninja-build clang
```

In addition, to get `cargo` and `rustc` you can use [rustup](https://rustup.rs/):

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

All the Rust code for the host has its dependencies indicated in the `Cargo.toml` file. Some dependencies require some shared libraries, which you can install with:

```bash
sudo apt install -y libusb-1.0-0-dev libftdi1-dev libudev-dev
```

It is worth mentioning that it is possible to avoid installing these dependencies as the build can be run within a docker container which already has all these dependencies available. Go to the [running in a docker container section](#running-in-a-docker-container) for more details on how to run the build within docker.

## Getting the Source

This project is [hosted on GitHub](https://github.com/Javier-varez/Postform).

You can get the repository by cloning this repository:

```bash
git clone https://github.com/Javier-varez/Postform postform
```

## Building

As mentioned before, [cargo xtasks](https://github.com/matklad/cargo-xtask) are used to automate build and testing processes. The following commands are available:

  * `cargo xtask build-firmware cortex_m3` - Builds an example app using libpostform for a Cortex-M3 MCU. Will be available under `fw_build/m3/app/postform_format`.
  * `cargo xtask build-firmware cortex_m0` - Builds an example app using libpostform for a Cortex-M0 MCU. Will be available under `fw_build/m0/app/postform_format`.
  * `cargo xtask test` - Runs a all Postform tests.
  * `cargo xtask build --release` - Builds all the host binaries like `postform_rtt`, `postform_persist` and `postform_serial`.
  * `cargo xtask clean` - Cleans all target build folders.
  * `cargo xtask run-example-app` - Runs the example application on an STM32F103C8 microcontroller. Should be connected using an ST-Link or compatible SWD debugger.

For more information about all available commands run `cargo xtask --help`.

## Usage

In order to run the example application run the following command:
```bash
cargo xtask run-example-app
```

This will build the example firmware application for the cortex-m3 and then immediately trigger `postform_rtt` (building it if needed). Then `postform_rtt` will connect to the target MCU via RTT (using an SWD connection) and load all debugging information from the FW ELF file. The example app is built for an `STM32F103C8` microcontrollers connected via a debugger compatible with `probe-rs`, like an `ST-Link` or a `J-Link`.

```bash
$ cargo xtask run-example-app
    Finished dev [unoptimized + debuginfo] target(s) in 0.01s
     Running `xtask/target/debug/xtask run-example-app`
$ cmake -G Ninja -DPOSTFORM_BUILD_EXAMPLES=true -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/armv7m.cmake -DPOSTFORM_BUILD_TARGET_APP=true
-- The C compiler identification is Clang 12.0.1
-- The CXX compiler identification is Clang 12.0.1
-- Check for working C compiler: /usr/bin/clang
-- Check for working C compiler: /usr/bin/clang -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/clang++
-- Check for working CXX compiler: /usr/bin/clang++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: /home/javier/Documents/code/Postform/fw_build/m3
$ cmake --build .
[14/14] Linking CXX executable app/postform_format
$ cargo run --bin=postform_rtt -- --chip STM32F103C8 fw_build/m3/app/postform_format
    Finished dev [unoptimized + debuginfo] target(s) in 0.05s
     Running `target/debug/postform_rtt --chip STM32F103C8 fw_build/m3/app/postform_format`
0.000001     Debug      : Iteration number: 0
└── File: ../../app/src/main.cpp, Line number: 86
0.000355     Debug      : Is this nice or what?!
└── File: ../../app/src/main.cpp, Line number: 88
0.000374     Info       : I am 28 years old...
└── File: ../../app/src/main.cpp, Line number: 89
0.000388     Warning    : Third string! With multiple args and more numbers: -1124
└── File: ../../app/src/main.cpp, Line number: 91
0.000412     Error      : Oh boy, error 234556 just happened
└── File: ../../app/src/main.cpp, Line number: 92
0.000429     Error      : This is my char array: 123
└── File: ../../app/src/main.cpp, Line number: 94
0.000447     Error      : different unsigned sizes: 123, 43212, 123123123, 123123123, 123123123
└── File: ../../app/src/main.cpp, Line number: 100
0.000484     Error      : different signed sizes: -123, -13212, -123123123, -123123123, -123123123
└── File: ../../app/src/main.cpp, Line number: 104
0.000528     Error      : different octal sizes: 123, 123, 123123, 123123123, 123123123
└── File: ../../app/src/main.cpp, Line number: 110
0.000561     Error      : different hex sizes: f3, 1321, 12341235, 12341234, 1234567812345678
└── File: ../../app/src/main.cpp, Line number: 116
0.000607     Error      : Pointer 0x12341234
└── File: ../../app/src/main.cpp, Line number: 117
0.000625     Debug      : Now if I wanted to print a really long text I can use %k: Lorem ipsum dolor sit amet, consectetur adipiscing
 eleifend quis convallis ut, venenatis quis mauris. Morbi tempor, ex a lobortis luctus, sem nunc laoreet dolor, pellentesque gravida m
pibus purus sed sagittis lobortis. Sed quis porttitor nulla. Nulla in ante ac arcu semper efficitur ut at erat. Fusce porttitor suscip
Morbi tristique tristique nulla, at posuere ex sagittis at. Aliquam est quam, porta nec erat ac, convallis tempus augue. Nam eu quam v
 Cras molestie eros odio, vitae ullamcorper ante vestibulum non. Vestibulum facilisis diam vel condimentum gravida. Donec in odio sit
tis laoreet. Nullam dignissim vel ex vel molestie. Vestibulum id eleifend metus. Curabitur malesuada condimentum augue ut molestie. Vi
└── File: ../../app/src/main.cpp, Line number: 149
1.000002     Debug      : Iteration number: 1
└── File: ../../app/src/main.cpp, Line number: 86
1.000454     Debug      : Is this nice or what?!
└── File: ../../app/src/main.cpp, Line number: 88
```

## Running in a docker container

Any of the `xtask` commands can be run within a docker container with all dependencies already installed. This is great if you don't want to go through the process of installing all dependencies listed in previous sections and just want a fast way to get started.

To run an `xtask` command in a docker container just insert `docker` after `xtask` in the command invocation. For instance, to run the tests in docker use:

```bash
cargo xtask docker test
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
