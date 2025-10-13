cmake_minimum_required(VERSION 3.16)
project(tfg_experimentos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Rutas del core
set(TFGCORE_INC "${CMAKE_SOURCE_DIR}/tfgcore/include")
set(TFGCORE_SRC "${CMAKE_SOURCE_DIR}/tfgcore/src")

# Incluye la carpeta de headers
include_directories("${TFGCORE_INC}")


file(GLOB_RECURSE CORE_SOURCES "${TFGCORE_SRC}/*.cpp")


add_executable(main ${CORE_SOURCES})


if (MSVC)
  target_compile_options(main PRIVATE /W4)
else()
  target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
endif()
