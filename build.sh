#!/bin/bash

# Global build options
CPPBASE_DEBUG=false
CPPBASE_STATIC_LIB=false
CPPBASE_ENABLE_MYSQL=false

# Set the right compiler depend on the environment
if [ "$OPENWRT_BUILD" ];then
        export CC=${TARGET_CC_NOCACHE}
        export CXX=${TARGET_CXX_NOCACHE}
        CMAKE_FLAGS="-DCMAKE_TOOLCHAIN_FILE=./cmake/ik_tool_chain.cmake"
else
        export CC=gcc
        export CXX=g++
fi

show_usage() {
	echo "OPTIONS"
	echo "    --clean: clean all build files"
	echo "    -d,--debug: build lib with debug info"
	echo "    --enable-mysql: build lib with mysql support"
	echo "    --enable-http: build lib with http support"
	echo "    -h,--help: show help"
	echo "    --make: make directly"
	echo "    -s,--static-lib: build lib as static lib"
	exit 0
}

build_clean() {
	make clean
	rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
	exit 0
}

# getopts doesn't support long option, so we did it by ourselves
while [ "$1" != "" ]; do
	case $1 in
		--clean)
			build_clean
		;;
		-d|--debug)
			CPPBASE_DEBUG=true
		;;
		--enable-mysql)
			CPPBASE_ENABLE_MYSQL=true
		;;
		--enable-http)
			CPPBASE_ENABLE_HTTP=true
		;;
		-h|-\?|--help)
			show_usage
		;;
		--make)
			make
			exit
		;;
		-s|--static-lib)
			CPPBASE_STATIC_LIB=true
		;;
		-?*)
			echo -e "\033[0;31m""Unknown option $1!!!""\033[0m" 
			show_usage
		;;
		*) # other options 
			echo -e "\033[0;31m""Forbid any other params!!!""\033[0m"
			show_usage
		;;
		
	esac
	shift
done


# Start set the cmake variables
if [ "$CPPBASE_DEBUG" = "true" ]; then
	echo "Build cppbase as debug version"
	CMAKE_FLAGS+=" -DCMAKE_BUILD_TYPE=Debug"
else
	echo "Build cppbase as release version"
fi

if [ "$CPPBASE_STATIC_LIB" = "true" ]; then
	echo "Build static libary"
	CMAKE_FLAGS+=" -DCPPBASE_STATIC_LIB=ON"
else
	echo "Build shared libary"
fi

if [ "$CPPBASE_ENABLE_MYSQL" = "true" ]; then
	echo "Enable MySQL support"
	CMAKE_FLAGS+=" -DCPPBASE_ENABLE_MYSQL=ON"
fi

if [ "$CPPBASE_ENABLE_HTTP" = "true" ]; then
	echo "Enable HTTP support"
	CMAKE_FLAGS+=" -DCPPBASE_ENABLE_HTTP=ON"
fi

mkdir -p deps/libs
tar -zxf deps/http-parser-2.7.1.tar.gz -C deps
make CC=${CC} -C deps/http-parser package
mv deps/http-parser/*.a  deps/libs

echo $CMAKE_FLAGS
rm -rf lib CMakeCache.txt CMakeFiles cmake_install.cmake
cmake ./ $CMAKE_FLAGS && make CC=${CC}
