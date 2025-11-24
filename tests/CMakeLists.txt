
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

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
