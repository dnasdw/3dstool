# 3dstool

An all-in-one tool for extracting/creating 3ds roms.

## History

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

## Platforms

- Cygwin
- Linux
- Mac OS X
- Windows *XP+*

## Building

### Dependencies

- cmake
- openssl-devel / libssl-dev
- libiconv **(linux only)**

### Compiling

- make 64-bit version
~~~
mkdir project
cd project
cmake ..
cmake ..
make
~~~

- make 32-bit version
~~~
mkdir project
cd project
cmake -DBUILD64=OFF ..
cmake ..
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
