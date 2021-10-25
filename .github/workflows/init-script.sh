#!/bin/bash -xe

# Install arm-none-eabi-gcc
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
tar -xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

# Add it to the path
export PATH=${PWD}/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}

# Install other dependencies
sudo apt-get update
sudo apt install -y libusb-1.0-0-dev libftdi1-dev libudev-dev ninja-build
