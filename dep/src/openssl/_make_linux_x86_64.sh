#!/bin/sh

rootdir=`dirname $0`
cd $rootdir
rootdir=`pwd`
target=x86_64
prefix=$rootdir/$target
openssldir=$prefix/ssl
openssl_version=`cat $rootdir/openssl.txt`
rm -rf "$rootdir/$openssl_version"
mkdir "$rootdir/$openssl_version"
cp -rf "$rootdir/../$openssl_version/"* "$rootdir/$openssl_version"
cd "$rootdir/$openssl_version"
./Configure --prefix="$prefix" --openssldir="$openssldir" linux-x86_64 -m64 -fPIC
make
make install
rm -rf "$rootdir/../../include/$target/openssl"
cp -rf "$prefix/include/"* "$rootdir/../../include/$target"
cp -f "$prefix/lib/libcrypto.a" "$rootdir/../../lib/$target"
cd $rootdir
rm -rf "$rootdir/$openssl_version"
