# 3dstool

An all-in-one tool for extracting/creating 3ds roms.

## History

- v1.2.0 @ 2018.06.03 - Support auto encryption fully
- v1.2.1 @ 2018.07.26 - Support openssl 1.1.0
- v1.2.2 @ 2018.08.27 - Fix not encrypt bug
- v1.2.3 @ 2018.09.03 - Fix extract cxi without exefs bug
- v1.2.4 @ 2019.03.28 - Improve lock region
- v1.2.5 @ 2019.04.20 - Fix extract exefs without exefs-dir bug
- v1.2.6 @ 2019.05.25 - Sync with libsundaowen_src

### v1.1

- v1.1.0 @ 2018.01.03 - A new beginning
- v1.1.1 @ 2018.01.21 - Lock region and language

### v1.0

- v1.0.0 @ 2014.12.07 - The very first release
- v1.0.1 @ 2014.12.09 - Make compatible with Windows XP
- v1.0.2 @ 2014.12.25 - Support diff and patch
- v1.0.3 @ 2014.12.26 - Fix diff bug
- v1.0.4 @ 2014.12.28 - Fix exefs header bug
- v1.0.5 @ 2015.01.05 - Fix diff bug
- v1.0.6 @ 2015.01.22 - Fix create romfs with reference bug
- v1.0.7 @ 2015.01.23 - Support banner
- v1.0.8 @ 2015.08.30 - Refactoring compression and fix banner bug
- v1.0.9 @ 2015.10.28 - Support romfs level3 only
- v1.0.10 @ 2015.11.22 - Support exefs in cfa and abandon romfs level3 only
- v1.0.11 @ 2016.02.04 - Fix diff bug and support VS2015
- v1.0.12 @ 2016.04.14 - Support 7.x auto encryption
- v1.0.13 @ 2016.04.20 - Fix romfs 7.x auto encryption bug
- v1.0.14 @ 2016.05.17 - Sync with exepatch and support VS2015
- v1.0.15 @ 2016.05.23 - Support auto encryption with ext key
- v1.0.16 @ 2016.11.01 - Support huffman, runlength, yaz0 compression, romfs remap ignore and VS2008SP1
- v1.0.17 @ 2016.11.06 - Fix romfs hash bug
- v1.0.18 @ 2016.12.04 - Compatible with yaz0 with alignment property
- v1.0.19 @ 2017.01.11 - Fix romfs hash bug
- v1.0.20 @ 2017.03.24 - Refactoring
- v1.0.21 @ 2017.04.02 - Refactoring
- v1.0.22 @ 2017.04.05 - Commandline support unicode
- v1.0.23 @ 2017.05.09 - Fix runtime error
- v1.0.24 @ 2017.05.21 - Support auto encryption
- v1.0.25 @ 2017.06.18 - Fix auto encryption bug and add download
- v1.0.26 @ 2017.06.20 - Fix auto encryption bug
- v1.0.27 @ 2017.10.10 - Fix encoding on macOS

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- libiconv
- openssl-devel / libssl-dev
- libcurl-devel

### Compiling

- make 64-bit version
~~~
mkdir build
cd build
cmake -DUSE_DEP=OFF ..
make
~~~

- make 32-bit version
~~~
mkdir build
cd build
cmake -DBUILD64=OFF -DUSE_DEP=OFF ..
make
~~~

### Installing

~~~
make install
~~~

## Usage

~~~
3dstool [option...] [option]...
~~~

## Options

See `3dstool --help` messages.
