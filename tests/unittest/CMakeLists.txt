include(CheckCXXCompilerFlag)

set(UNITTEST_SOURCES
	unittest.cc
	utils-test.cc)

find_program(CCACHE_FOUND ccache)

set_property(DIRECTORY PROPERTY COMPILE_OPTIONS ${EXTRA_CXX_FLAGS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_HAS_STDSTRING=1")

add_executable(unittest ${UNITTEST_SOURCES})
target_link_libraries(unittest ${TEST_LIBRARIES})

add_dependencies(tests unittest)

add_test(NAME unittest
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unittest)
