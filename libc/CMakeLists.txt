cmake_minimum_required(VERSION 3.4.3)

# Use old version of target_sources command which converts the source
# file paths to full paths.
cmake_policy(SET CMP0076 OLD)
list(APPEND CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# The top-level source directory of libc.
set(LIBC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# The top-level directory in which libc is being built.
set(LIBC_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Path libc/scripts directory.
set(LIBC_BUILD_SCRIPTS_DIR "${LIBC_SOURCE_DIR}/utils/build_scripts")


set(LIBC_TARGET_OS ${CMAKE_SYSTEM_NAME})
string(TOLOWER ${LIBC_TARGET_OS} LIBC_TARGET_OS)

set(LIBC_TARGET_MACHINE ${CMAKE_SYSTEM_PROCESSOR})

include(CMakeParseArguments)
include(LLVMLibCRules)

add_subdirectory(src)
add_subdirectory(config)
add_subdirectory(include)
add_subdirectory(utils)

# The lib and test directories are added at the very end as tests
# and libraries potentially draw from the components present in all
# of the other directories.
add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(fuzzing)
