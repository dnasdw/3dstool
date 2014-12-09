# 3DS Tool

An all-in-one tool for extracting/creating 3ds roms.

## History

- v1.0.0 @ 2014.12.07 - The very first release

## Platforms

- Cygwin *1.7.33+*
- Linux *(Tested on Ubuntu 14.04+, CentOS 6.3+)*
- Mac OS X *10.9+*
- Windows *Vista+*

## Building

> A 64-bit system is required for building and running this tool. 

### Dependencies

- cmake
- openssl-devel / libssl-dev
- libiconv **(linux only)**

### Compiling

~~~
mkdir projectcd projectcmake ..make
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