#!/bin/bash

pushd "`dirname "$0"`"
tmpdir=/tmp/3dstool_openssl
target=linux_x86_64
prefix=$tmpdir/$target
openssldir=$prefix/ssl
version=`cat version.txt`
rm -rf "$tmpdir/$version"
mkdir -p "$tmpdir/$version"
cp -rf "../$version/"* "$tmpdir/$version"
pushd "$tmpdir/$version"
./Configure no-shared no-asm --prefix="$prefix" --openssldir="$openssldir" linux-generic64 -m64 -fPIC
make
make install
popd
mkdir -p "../../include/$target"
cp -rf "$prefix/include/"* "../../include/$target"
mkdir -p "../../lib/$target"
cp -f "$prefix/lib/"*.a "../../lib/$target"
popd
rm -rf "$tmpdir"
