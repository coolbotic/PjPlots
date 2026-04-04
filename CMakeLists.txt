cmake_minimum_required(VERSION 3.20)

project(PjPlots VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(PjPlots INTERFACE)
add_library(PjPlots::PjPlots ALIAS PjPlots)

target_compile_features(PjPlots INTERFACE cxx_std_20)

target_include_directories(PjPlots
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

option(PJPLOTS_ENABLE_TESTS "Enable PjPlots compile-time tests" OFF)

if(PJPLOTS_ENABLE_TESTS)
    target_compile_definitions(PjPlots INTERFACE PJPLOT_ENABLE_TESTS)
endif()

install(
    TARGETS PjPlots
    EXPORT PjPlotsTargets
)

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/PjPlots.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PjPlots
)

install(
    EXPORT PjPlotsTargets
    FILE PjPlotsTargets.cmake
    NAMESPACE PjPlots::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PjPlots
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PjPlotsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PjPlotsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PjPlots
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PjPlotsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/PjPlotsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/PjPlotsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PjPlots
)
