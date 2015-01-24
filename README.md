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

## Platforms

- Cygwin *1.7.33+*
- Linux *(Tested on Ubuntu 14.04+, CentOS 6.3+)*
- Mac OS X *10.9+*
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
cmake -DBUILD64=OFF ..
make
~~~

### Installing

~~~
make install
~~~

## Usage

### Windows

~~~
3dstool.exe [option...] [option]...
~~~

### Other

~~~
3dstool [option...] [option]...
~~~

> Remember to do `chmod +x 3dstool` first

## Options

See `3dstool --help` messages.

## FAQ

Nothing here for now.
