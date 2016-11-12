cmake_minimum_required(VERSION 3.6)
project(ResearchMac)

enable_language(C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=gnu11")

set(SOURCE_FILES osversion.c)
add_executable(ResearchMac ${SOURCE_FILES})

set(CMAKE_CXX_FLAGS "$(CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -headerpad_max_install_names -framework CoreFoundation \
-framework IOKit -framework CoreServices")