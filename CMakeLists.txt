cmake_minimum_required(VERSION 3.10)

set(PROJECT "klvparse")
set(VERSION "1.0.0")
project("${PROJECT}" VERSION "${VERSION}")


# nlohmann_json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann_json")

## GStreamer + WebRTC related dependencies
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    "gstreamer-1.0"
    "gstreamer-base-1.0"
    "gstreamer-app-1.0"
    "gstreamer-rtp-1.0"
    "gstreamer-audio-1.0"
    "gstreamer-video-1.0"
    "gstreamer-pbutils-1.0"
    "gstreamer-tag-1.0"
    "gstreamer-webrtc-1.0"
    "gstreamer-sdp-1.0"
)

set(TARGET "${PROJECT}")

message(STATUS "version: ${${TARGET}_VERSION}")

add_subdirectory("plugins")