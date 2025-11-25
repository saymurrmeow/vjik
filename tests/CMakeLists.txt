
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage")
  set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage")
  set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
  set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage")
  set(CMAKE_MODULE_LINKER_FLAGS_COVERAGE "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} --coverage")
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory coverage
    COMMAND ${GCOVR_PATH} -r ${CMAKE_SOURCE_DIR} -e '${CMAKE_SOURCE_DIR}/tests/' -e '${CMAKE_SOURCE_DIR}/build/' --html --html-nested -o coverage/index.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating HTML coverage report..."
  )
endif()

enable_testing()
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(
  tests
  PRIVATE
    vjnet 
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
