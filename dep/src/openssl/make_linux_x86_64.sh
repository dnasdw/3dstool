#!/bin/bash

cwdir=`pwd`
rootdir=`dirname "$0"`
cd "$rootdir"
rootdir=`pwd`
tmpdir=/tmp/3dstool_openssl
target=linux_x86_64
prefix=$tmpdir/$target
openssldir=$prefix/ssl
version=`cat "$rootdir/version.txt"`
rm -rf "$tmpdir/$version"
mkdir -p "$tmpdir/$version"
cp -rf "$rootdir/../$version/"* "$tmpdir/$version"
cd "$tmpdir/$version"
./Configure no-shared no-asm no-dso --prefix="$prefix" --openssldir="$openssldir" linux-generic64 -m64 -fPIC
make
make install
mkdir -p "$rootdir/../../include/$target"
cp -rf "$prefix/include/"* "$rootdir/../../include/$target"
mkdir -p "$rootdir/../../lib/$target"
cp -f "$prefix/lib/"*.a "$rootdir/../../lib/$target"
cd "$cwdir"
rm -rf "$tmpdir"
rm -rf "$prefix"
