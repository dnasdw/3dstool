#!/bin/bash

cwdir=`pwd`
rootdir=`dirname "$0"`
cd "$rootdir"
rootdir=`pwd`
target=linux_x86_64
prefix=$rootdir/$target
version=`cat "$rootdir/version.txt"`
rm -rf "$rootdir/$version"
mkdir -p "$rootdir/$version"
cp -rf "$rootdir/../$version/"* "$rootdir/$version"
rm -rf "$rootdir/build"
mkdir -p "$rootdir/build"
cd "$rootdir/build"
openssl_version=`cat "$rootdir/../openssl/version.txt"`
cmake -DBUILD64=ON -C "$rootdir/CMakeLists.txt" -D_OPENSSL_VERSION="$openssl_version" -DOPENSSL_INCLUDE_DIR="$rootdir/../../include/$target" -DOPENSSL_CRYPTO_LIBRARY="$rootdir/../../lib/$target/libcrypto.a" -DOPENSSL_SSL_LIBRARY="$rootdir/../../lib/$target/libssl.a" -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DCURL_STATICLIB=ON -DCURL_DISABLE_LDAP=ON -DCURL_ZLIB=OFF -DCMAKE_INSTALL_PREFIX="$prefix" "$rootdir/$version"
cmake --build . --target install --config Release --clean-first
mkdir -p "$rootdir/../../include/$target"
cp -rf "$prefix/include/"* "$rootdir/../../include/$target"
mkdir -p "$rootdir/../../lib/$target"
cp -f "$prefix/lib/libcurl.a" "$rootdir/../../lib/$target"
cd "$cwdir"
rm -rf "$rootdir/$version"
rm -rf "$rootdir/build"
rm -rf "$prefix"
