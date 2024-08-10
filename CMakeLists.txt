cmake_minimum_required(VERSION 3.5)

project(noteutil)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

add_definitions("-Wall" "-Wextra" "-pedantic")

set(UNFORMATTER_INSTALL_CMAKE_DEST "${CMAKE_INSTALL_LIBDIR}/cmake/xarray")

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/UnformatterConfig.cmake"
    INSTALL_DESTINATION "${UNFORMATTER_INSTALL_CMAKE_DEST}")

install(EXPORT UnformatterTargets NAMESPACE unformatter::
    FILE "UnformatterTargets.cmake"
    DESTINATION "${UNFORMATTER_INSTALL_CMAKE_DEST}")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/UnformatterConfig.cmake"
    DESTINATION "${UNFORMATTER_INSTALL_CMAKE_DEST}")

add_subdirectory(unformatter)
add_subdirectory(sample)
