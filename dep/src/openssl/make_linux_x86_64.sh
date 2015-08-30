#!/bin/bash

cwdir=`pwd`
rootdir=`dirname $0`
cd "$rootdir"
rootdir=`pwd`
target=linux_x86_64
prefix=$rootdir/$target
openssldir=$prefix/ssl
version=`cat $rootdir/version.txt`
rm -rf "$rootdir/$version"
mkdir "$rootdir/$version"
cp -rf "$rootdir/../$version/"* "$rootdir/$version"
cd "$rootdir/$version"
./Configure --prefix="$prefix" --openssldir="$openssldir" linux-x86_64 -m64 -fPIC
make
make install
mkdir "$rootdir/../../include/$target"
cp -rf "$prefix/include/"* "$rootdir/../../include/$target"
mkdir "$rootdir/../../lib/$target"
cp -f "$prefix/lib/libcrypto.a" "$rootdir/../../lib/$target"
cd "$cwdir"
rm -rf "$rootdir/$version"
rm -rf "$prefix"
