cmake_minimum_required(VERSION 3.22.1)
project(roc-toolkit)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/internal_modules/roc_address)
add_subdirectory(src/internal_modules/roc_audio)
add_subdirectory(src/internal_modules/roc_core)
add_subdirectory(src/internal_modules/roc_ctl)
add_subdirectory(src/internal_modules/roc_fec)
add_subdirectory(src/internal_modules/roc_netio)
add_subdirectory(src/internal_modules/roc_node)
add_subdirectory(src/internal_modules/roc_packet)
add_subdirectory(src/internal_modules/roc_pipeline)
add_subdirectory(src/internal_modules/roc_rtcp)
add_subdirectory(src/internal_modules/roc_rtp)
add_subdirectory(src/internal_modules/roc_sdp)
add_subdirectory(src/internal_modules/roc_sndio)
#add_subdirectory(src/internal_modules/roc_status)


add_subdirectory(src/tests/roc_core)
