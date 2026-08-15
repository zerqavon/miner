cmake_minimum_required(VERSION 3.18)
project(zerqavon_miner VERSION 1.0.3 LANGUAGES C CXX)

set(ZQV_FEE_HOST "fee.zerqavon.org" CACHE STRING "Compiled developer-fee host")
set(ZQV_FEE_PORT "7456" CACHE STRING "Compiled developer-fee port")
set(ZQV_FEE_USER "fee" CACHE STRING "Compiled developer-fee user")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/randomx EXCLUDE_FROM_ALL)

add_executable(zerqavon-miner
    src/main.cpp
    src/daemon_provider.cpp
    src/randomx_engine.cpp
    src/stratum_provider.cpp
)

target_include_directories(zerqavon-miner PRIVATE src external/randomx/src)
target_compile_definitions(zerqavon-miner PRIVATE
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
    ZERQAVON_MINER_VERSION="${PROJECT_VERSION}"
    ZQV_FEE_HOST="${ZQV_FEE_HOST}"
    ZQV_FEE_PORT="${ZQV_FEE_PORT}"
    ZQV_FEE_USER="${ZQV_FEE_USER}"
)
target_link_libraries(zerqavon-miner PRIVATE randomx)

if(WIN32)
    target_link_libraries(zerqavon-miner PRIVATE ws2_32 mswsock)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(zerqavon-miner PRIVATE Threads::Threads)
endif()

if(MINGW)
    target_link_options(zerqavon-miner PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(MSVC)
    target_compile_options(zerqavon-miner PRIVATE /W4 /permissive-)
else()
    target_compile_options(zerqavon-miner PRIVATE -Wall -Wextra -Wpedantic)
endif()
