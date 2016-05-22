#!/bin/bash

cwdir=`pwd`
rootdir=`dirname $0`
cd "$rootdir"
rootdir=`pwd`
target=linux_x86_32
prefix=$rootdir/$target
openssldir=$prefix/ssl
version=`cat $rootdir/version.txt`
rm -rf "$rootdir/$version"
mkdir "$rootdir/$version"
cp -rf "$rootdir/../$version/"* "$rootdir/$version"
cd "$rootdir/$version"
./Configure no-shared no-asm no-dso --prefix="$prefix" --openssldir="$openssldir" linux-generic32 -m32 -fPIC
make
make install
mkdir "$rootdir/../../include/$target"
cp -rf "$prefix/include/"* "$rootdir/../../include/$target"
mkdir "$rootdir/../../lib/$target"
cp -f "$prefix/lib/"*.a "$rootdir/../../lib/$target"
cd "$cwdir"
rm -rf "$rootdir/$version"
rm -rf "$prefix"
