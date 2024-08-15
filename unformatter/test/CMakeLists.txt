include(FindPkgConfig)

set(NAME "test_unformatter")

find_package(Catch2 REQUIRED)

include(Catch)

file(GLOB SRCS "*.cpp")

add_executable(${NAME} ${SRCS})
target_link_libraries(${NAME}
    PRIVATE ${UNFORMATTER_PRIV} Catch2::Catch2WithMain)

catch_discover_tests(${NAME}
    DISCOVERY_MODE PRE_TEST)
