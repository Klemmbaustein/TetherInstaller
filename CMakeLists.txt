cmake_minimum_required(VERSION 3.15)

project(TetherInstaller)

set(CMAKE_CXX_STANDARD 20)

set(SRC_DIR "NorthstarInstaller/Source")

file(
    GLOB_RECURSE
    SRCS
    "${SRC_DIR}/**.cpp"
)
list(APPEND SRCS "tinyfiledialogs/tinyfiledialogs.c")

add_subdirectory("KlemmUI/")
add_subdirectory("json")

find_package(CURL)

# TODO: use the curl submodule.
# Right now, this will only use pre-installed libcurl.
# To fix this, I would need to figure out compiling it with openssl.
# On windows, CURL compiles with CURL_USE_SCHANNEL. (This will not work on linux)
if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
else(CURL_FOUND)
    message(FATAL_ERROR "Could not find the CURL library and development files.")
endif(CURL_FOUND)

add_executable(
   TetherInstaller
   ${SRCS}
)

if (DEFINED TETHER_CI)
    add_compile_definitions(CI_BUILD=${TETHER_CI})
endif()

target_include_directories(TetherInstaller PRIVATE "tinyfiledialogs/")
target_include_directories(TetherInstaller PRIVATE "Shared/miniz")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-char8_t" )

target_link_libraries(TetherInstaller PRIVATE SDL2::SDL2)
target_link_libraries(TetherInstaller PRIVATE libglew_static)
target_link_libraries(TetherInstaller PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(TetherInstaller PRIVATE KlemmUI)
target_link_libraries(TetherInstaller PRIVATE ${CURL_LIBRARIES})
