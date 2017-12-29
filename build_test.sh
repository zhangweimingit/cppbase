#!/bin/bash

if [ "$1" = "make" ];then
cd build
make
cd ..
exit 0
fi

rm -rf build/ deps/http-parser/ deps/libs/
if [ "$1" = "clean" ];then
    exit 0
fi

# Cross Compiling
CC="gcc"
if [ "$1" = "OpenWrt" ];then
    source ./cmake/openwrt.config
    CC="i486-openwrt-linux-uclibc-gcc"
fi

# build http-parser
cd deps
mkdir libs 
tar -zxf http-parser-2.7.1.tar.gz
make CC=${CC} -C http-parser/ package
mv http-parser/*.a libs/
cd ..

# cmake
mkdir build
cd build
if [ "$1" = "OpenWrt" -a "$2" = "" ];then
cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/ik_tool_chain.cmake ..
elif [ "$1" = "OpenWrt" -a "$2" = "Debug" ];then 
cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/ik_tool_chain.cmake -DCMAKE_BUILD_TYPE=Debug ..
elif [ "$1" = "Debug" ];then
cmake -DCMAKE_BUILD_TYPE=Debug ..
else
cmake ..
fi

# make
#make static_check
#if [ $? -ne 0 ];then
#    exit 1
#fi
make
