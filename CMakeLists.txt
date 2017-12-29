#
#CMAKE_BUILD_TYPE: "Debug" or "Release", default is "Release"
#CPPBASE_STATIC_LIBS: "ON" or "OFF", default is "OFF"
#CPPBASE_ENABLE_MYSQL: "ON" or "OFF", default is "OFF"
#


cmake_minimum_required (VERSION 2.6)
project(cppbase)

# Set build option as default value
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/ik_static_check.cmake)

# cpplint
file(GLOB_RECURSE ALL_CHECK_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_static_check_target(static_check "${ALL_CHECK_FILES}")

# Check the dep lib
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	add_compile_options(-pthread)
endif()
if(CMAKE_THREAD_LIBS_INIT)
	link_libraries(${CMAKE_THREAD_LIBS_INIT})
endif()

set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set(CPPBASE_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(CPPBASE_DEPS_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(CPPBASE_DEP_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/libs)
set(CPPBASE_SYSTEM_INC_DIR)

include_directories(${CPPBASE_INC_DIR})
include_directories(${CPPBASE_DEPS_INC_DIR})
include_directories(SYSTEM ${CPPBASE_SYSTEM_INC_DIR})
link_directories(${CPPBASE_DEP_LIB_DIR})

file(GLOB_RECURSE CPPBASE_BASE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/base/*.cpp")
file(GLOB_RECURSE CPPBASE_CORE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp")
# Add option codes
if(CPPBASE_ENABLE_MYSQL MATCHES "ON")
	file(GLOB_RECURSE CPPBASE_DB_MYSQL_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/db/mysql/*.cpp")
endif()
set(CPPBASE_DB_SRCS ${CPPBASE_DB_MYSQL_SRCS})

set(CPPBASE_SRCS ${CPPBASE_BASE_SRCS} ${CPPBASE_CORE_SRCS} ${CPPBASE_DB_SRCS})

# Build liberary
if(CPPBASE_STATIC_LIB MATCHES "ON")
        if(CPPBASE_ENABLE_HTTP MATCHES "ON")
                list(APPEND CPPBASE_DEP_LIBS ${CPPBASE_DEP_LIB_DIR}/libhttp_parser.a)
        endif()

	add_library(cppbase_lib_tmp STATIC ${CPPBASE_SRCS})
	add_custom_target(cppbase_lib ALL
		COMMAND ar -rcT lib/libcppbase.a lib/libcppbase_lib_tmp.a ${CPPBASE_DEP_LIBS}
		DEPENDS cppbase_lib_tmp)

	set_target_properties(cppbase_lib PROPERTIES OUTPUT_NAME "cppbase")
else()
	add_library(cppbase_lib SHARED ${CPPBASE_SRCS})

	if(CPPBASE_ENABLE_HTTP MATCHES "ON")
		list(APPEND CPPBASE_DEP_LIBS http_parser)
	endif()
	target_link_libraries(cppbase_lib ${CPPBASE_DEP_LIBS})
endif()
set_target_properties(cppbase_lib PROPERTIES OUTPUT_NAME "cppbase")
set_target_properties(cppbase_lib PROPERTIES VERSION 1.0.0 SOVERSION 1)

