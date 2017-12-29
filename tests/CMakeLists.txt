find_package(GTestSrc)

IF(GTESTSRC_FOUND)
	enable_testing()

	add_subdirectory(${GTEST_SOURCE_DIR} ${CMAKE_BINARY_DIR}/googletest)
	include_directories(SYSTEM ${GTEST_INCLUDE_DIR})

	set(TEST_LIBRARIES gtest gtest_main cppbase)

	add_custom_target(tests ALL)
	add_subdirectory(unittest)

ENDIF(GTESTSRC_FOUND)
