cmake_minimum_required(VERSION 3.24)
project(oscpp VERSION 1.0.0 LANGUAGES CXX)

add_library(oscpp INTERFACE)
add_library(oscpp::oscpp ALIAS oscpp)

target_include_directories(oscpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(oscpp INTERFACE cxx_std_11)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS oscpp EXPORT oscppTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
    EXPORT oscppTargets
    FILE oscppConfig.cmake
    NAMESPACE oscpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/oscpp
)
write_basic_package_version_file(
    oscppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/oscppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/oscpp
)

option(OSCPP_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
if(OSCPP_BUILD_TESTS)
    include(CTest)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_subdirectory(test)
endif()
