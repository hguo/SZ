cmake_minimum_required (VERSION 3.1.3)
project (sz)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

find_package (ZLIB REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/sz/include)
include_directories (${ZLIB_INCLUDE_DIRS})

add_subdirectory (sz)
add_subdirectory (example)
