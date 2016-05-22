#!/bin/bash

cwdir=`pwd`
rootdir=`dirname $0`
cd "$rootdir"
rootdir=`pwd`
target=mac_x86_32
prefix=$rootdir/$target
version=`cat $rootdir/version.txt`
rm -rf "$rootdir/$version"
mkdir "$rootdir/$version"
cp -rf "$rootdir/../$version/"* "$rootdir/$version"
rm -rf "$rootdir/project"
mkdir "$rootdir/project"
cd "$rootdir/project"
openssl_version=`cat $rootdir/../openssl/version_for_curl.txt`
cmake -D_OPENSSL_VERSION="$openssl_version" -DOPENSSL_INCLUDE_DIR="$rootdir/../../include/$target" -DOPENSSL_CRYPTO_LIBRARY="$rootdir/../../lib/$target/libcrypto.a" -DOPENSSL_SSL_LIBRARY="$rootdir/../../lib/$target/libssl.a" -DBUILD_CURL_EXE=OFF -DBUILD_CURL_TESTS=OFF -DCURL_STATICLIB=ON -DCMAKE_INSTALL_PREFIX="$prefix" "$rootdir/$version"
cmake --build . --target install --config Release --clean-first
mkdir "$rootdir/../../include/$target"
cp -rf "$prefix/include/"* "$rootdir/../../include/$target"
mkdir "$rootdir/../../lib/$target"
cp -f "$prefix/lib/libcurl.a" "$rootdir/../../lib/$target"
cd "$cwdir"
rm -rf "$rootdir/$version"
rm -rf "$rootdir/project"
rm -rf "$prefix"
