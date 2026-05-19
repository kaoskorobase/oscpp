include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
)
FetchContent_Declare(rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG        b2d9ed2dddefc4b84318d664b4f221eb792d89c7
)
# RC_ENABLE_CATCH also pulls in rapidcheck's bundled Catch v2 (ext/catch), which
# conflicts with the FetchContent'd Catch2 v3. Keep it OFF and add the
# Catch2-v3-compatible extras/catch subdirectory manually after population.
set(RC_ENABLE_CATCH OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Catch2 rapidcheck)
add_subdirectory(${rapidcheck_SOURCE_DIR}/extras/catch
                 ${CMAKE_CURRENT_BINARY_DIR}/rapidcheck_catch)

set(oscpp_warnings -Wall -Wextra -Wno-unused-parameter)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_executable(oscpp_autocheck oscpp_autocheck.cpp)
target_link_libraries(oscpp_autocheck PRIVATE
    oscpp::oscpp Catch2::Catch2WithMain rapidcheck rapidcheck_catch)
target_compile_options(oscpp_autocheck PRIVATE
    "$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:${oscpp_warnings}>")
include(Catch)
catch_discover_tests(oscpp_autocheck)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/README.cpp
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tools/mdcode.py
            ${CMAKE_SOURCE_DIR}/README.md
            ${CMAKE_CURRENT_BINARY_DIR}/README.cpp
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/README.md
)
add_executable(oscpp_readme ${CMAKE_CURRENT_BINARY_DIR}/README.cpp)
target_include_directories(oscpp_readme PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(oscpp_readme PRIVATE
    "$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:${oscpp_warnings}>")
add_test(NAME oscpp_readme COMMAND oscpp_readme)
