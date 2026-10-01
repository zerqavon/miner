cmake_minimum_required(VERSION 3.10)
project(liquidminer)

option(WITH_HWLOC           "Enable hwloc support" ON)
option(WITH_CN_LITE         "Enable CryptoNight-Lite algorithms family" ON)
option(WITH_CN_HEAVY        "Enable CryptoNight-Heavy algorithms family" ON)
option(WITH_CN_PICO         "Enable CryptoNight-Pico algorithm" ON)
option(WITH_CN_FEMTO        "Enable CryptoNight-UPX2 algorithm" ON)
option(WITH_RANDOMX         "Enable RandomX algorithms family" ON)
option(WITH_ARGON2          "Enable Argon2 algorithms family" ON)
option(WITH_KAWPOW          "Enable KawPow algorithms family" ON)
option(WITH_GHOSTRIDER      "Enable GhostRider algorithm" ON)
option(WITH_HTTP            "Enable HTTP protocol support (client/server)" ON)
option(WITH_DEBUG_LOG       "Enable debug log output" OFF)
option(WITH_TLS             "Enable OpenSSL support" ON)
option(WITH_ASM             "Enable ASM PoW implementations" ON)
option(WITH_MSR             "Enable MSR mod & 1st-gen Ryzen fix" ON)
option(WITH_ENV_VARS        "Enable environment variables support in config file" ON)
option(WITH_EMBEDDED_CONFIG "Enable internal embedded JSON config" OFF)
option(WITH_OPENCL          "Enable OpenCL backend" ON)
set(WITH_OPENCL_VERSION 200 CACHE STRING "Target OpenCL version")
set_property(CACHE WITH_OPENCL_VERSION PROPERTY STRINGS 120 200 210 220)
option(WITH_CUDA            "Enable CUDA backend" ON)
option(WITH_VULKAN_KAWPOW   "Enable Vulkan KawPow backend" ON)
option(WITH_PEARL_HIP       "Build AMD HIP PearlHash backend (requires ROCm HIP SDK)" OFF)
option(WITH_NEXA_HIP        "Enable native AMD HIP NexaPoW backend" ON)
option(WITH_VERUSHASH       "Enable VerusHash v2.2 CPU algorithm" ON)
option(WITH_XELISHASH       "Enable XelisHash v3 CPU algorithm" ON)
option(WITH_NVML            "Enable NVML (NVIDIA Management Library) support (only if CUDA backend enabled)" ON)
option(WITH_ADL             "Enable ADL (AMD Display Library) or sysfs support (only if OpenCL backend enabled)" ON)
option(WITH_STRICT_CACHE    "Enable strict checks for OpenCL cache" ON)
option(WITH_INTERLEAVE_DEBUG_LOG "Enable debug log for threads interleave" OFF)
option(WITH_PROFILING       "Enable profiling for developers" OFF)
option(WITH_SSE4_1          "Enable SSE 4.1 for Blake2" ON)
option(WITH_AVX2            "Enable AVX2 for Blake2" ON)
option(WITH_VAES            "Enable VAES instructions for Cryptonight" ON)
option(WITH_BENCHMARK       "Enable builtin benchmark and stress test" OFF)
option(WITH_SECURE_JIT      "Enable secure access to JIT memory" OFF)
option(WITH_DMI             "Enable DMI/SMBIOS reader" ON)

# Build a binary tuned for the host CPU. Keep this disabled for distributable
# binaries because the resulting executable may not run on older CPUs.
option(XMRIG_NATIVE_OPTIMIZATIONS "Tune native C/C++ code for the build host CPU" OFF)

option(BUILD_STATIC         "Build static binary" OFF)
option(ARM_V8               "Force ARMv8 (64 bit) architecture, use with caution if automatic detection fails, but you sure it may work" OFF)
option(ARM_V7               "Force ARMv7 (32 bit) architecture, use with caution if automatic detection fails, but you sure it may work" OFF)
option(HWLOC_DEBUG          "Enable hwloc debug helpers and log" OFF)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


include (CheckIncludeFile)
include (cmake/cpu.cmake)
include (cmake/os.cmake)
include (src/base/base.cmake)
include (src/backend/backend.cmake)

if (WITH_VULKAN_KAWPOW)
    if (WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        set(_vulkan_kawpow_default "${CMAKE_SOURCE_DIR}/thirdparty-bench/rdna3-kawpow-miner/rust/target/x86_64-pc-windows-gnu/release/librdna3_kawpow.a")
    else()
        set(_vulkan_kawpow_default "${CMAKE_SOURCE_DIR}/thirdparty-bench/rdna3-kawpow-miner/rust/target/release/rdna3_kawpow.lib")
    endif()
    set(VULKAN_KAWPOW_RUST_LIB "${_vulkan_kawpow_default}" CACHE FILEPATH "Release static library produced by the Vulkan KawPow host")
    if (NOT EXISTS "${VULKAN_KAWPOW_RUST_LIB}")
        message(WARNING "Vulkan KawPow library not found: ${VULKAN_KAWPOW_RUST_LIB}; Vulkan backend disabled")
        set(WITH_VULKAN_KAWPOW OFF)
    endif()
endif()

if (WITH_PEARL_HIP)
    if (NOT WIN32)
        message(FATAL_ERROR "WITH_PEARL_HIP currently supports the Windows HIP SDK layout only")
    endif()
    find_program(PEARL_HIPCC hipcc
        HINTS "$ENV{HIP_PATH}/bin" "C:/Program Files/AMD/ROCm/7.2/bin"
        PATH_SUFFIXES bin)
    if (NOT PEARL_HIPCC)
        message(FATAL_ERROR "WITH_PEARL_HIP requires hipcc from the AMD HIP SDK")
    endif()
    set(PEARL_HIP_SRC "${CMAKE_SOURCE_DIR}/thirdparty-bench/akoya-miner/native/pearl-gemm/csrc")
    set(PEARL_HIP_HOST "${PEARL_HIP_SRC}/rocm/host/pearl_gemm_capi_rocm.cpp")
    set(PEARL_HIP_DLL "${CMAKE_BINARY_DIR}/pearl_gemm_capi.dll")
    add_custom_command(
        OUTPUT "${PEARL_HIP_DLL}"
        COMMAND "${PEARL_HIPCC}" --offload-arch=gfx1200 -std=c++17 -O3 -shared
                -I "${PEARL_HIP_SRC}" -I "${PEARL_HIP_SRC}/rocm"
                "${PEARL_HIP_HOST}" -o "${PEARL_HIP_DLL}"
        DEPENDS "${PEARL_HIP_HOST}"
                "${PEARL_HIP_SRC}/rocm/pearl_kernels.cuh"
                "${PEARL_HIP_SRC}/rocm/blake3_device.cuh"
                "${PEARL_HIP_SRC}/capi/pearl_gemm_capi.h"
        VERBATIM)
    add_custom_target(liquidminer_pearl_hip ALL DEPENDS "${PEARL_HIP_DLL}")
endif()

if (WITH_NEXA_HIP AND WIN32)
    find_path(NEXA_HIP_INCLUDE hip/hip_runtime_api.h
        HINTS "$ENV{HIP_PATH}/include" "C:/Program Files/AMD/ROCm/7.2/include")
    find_library(NEXA_HIP_LIBRARY amdhip64
        HINTS "$ENV{HIP_PATH}/lib" "C:/Program Files/AMD/ROCm/7.2/lib")
    find_library(NEXA_HIPRTC_LIBRARY hiprtc
        HINTS "$ENV{HIP_PATH}/lib" "C:/Program Files/AMD/ROCm/7.2/lib")
    if (NEXA_HIP_INCLUDE AND NEXA_HIP_LIBRARY AND NEXA_HIPRTC_LIBRARY)
        add_definitions(/DXMRIG_FEATURE_NEXA_HIP /D__HIP_PLATFORM_AMD__)
        include_directories("${NEXA_HIP_INCLUDE}")
    else()
        message(WARNING "Native AMD HIP NexaPoW backend disabled: ROCm HIP SDK not found")
        set(WITH_NEXA_HIP OFF)
    endif()
endif()


set(HEADERS
    "${HEADERS_BASE}"
    "${HEADERS_BASE_HTTP}"
    "${HEADERS_BACKEND}"
    src/App.h
    src/core/config/Config_default.h
    src/core/config/Config_platform.h
    src/core/config/Config.h
    src/core/config/ConfigTransform.h
    src/core/config/usage.h
    src/core/Controller.h
    src/core/Miner.h
    src/core/Taskbar.h
    src/net/interfaces/IJobResultListener.h
    src/net/JobResult.h
    src/net/JobResults.h
    src/net/Network.h
    src/net/strategies/DonateStrategy.h
    src/Summary.h
    src/version.h
   )

set(HEADERS_CRYPTO
    src/backend/common/interfaces/IMemoryPool.h
    src/crypto/cn/asm/CryptonightR_template.h
    src/crypto/cn/c_blake256.h
    src/crypto/cn/c_groestl.h
    src/crypto/cn/c_jh.h
    src/crypto/cn/c_skein.h
    src/crypto/cn/CnAlgo.h
    src/crypto/cn/CnCtx.h
    src/crypto/cn/CnHash.h
    src/crypto/cn/CryptoNight_monero.h
    src/crypto/cn/CryptoNight_test.h
    src/crypto/cn/CryptoNight.h
    src/crypto/civiclight/CivicLight.h
    src/crypto/civiclight/yespower/insecure_memzero.h
    src/crypto/civiclight/yespower/sysendian.h
    src/crypto/civiclight/yespower/yespower.h
    src/crypto/civiclight/yespower/yespower_sha256.h
    src/crypto/cn/groestl_tables.h
    src/crypto/cn/hash.h
    src/crypto/cn/skein_port.h
    src/crypto/cn/soft_aes.h
    src/crypto/common/HugePagesInfo.h
    src/crypto/common/MemoryPool.h
    src/crypto/common/Nonce.h
    src/crypto/common/portable/mm_malloc.h
    src/crypto/common/VirtualMemory.h
    src/crypto/nexapow/NexaPow.h
   )

if (XMRIG_ARM OR XMRIG_RISCV)
    set(HEADERS_CRYPTO "${HEADERS_CRYPTO}" src/crypto/cn/CryptoNight_arm.h)
else()
    set(HEADERS_CRYPTO "${HEADERS_CRYPTO}" src/crypto/cn/CryptoNight_x86.h)
endif()

set(SOURCES
    "${SOURCES_BASE}"
    "${SOURCES_BASE_HTTP}"
    "${SOURCES_BACKEND}"
    src/App.cpp
    src/core/config/Config.cpp
    src/core/config/ConfigTransform.cpp
    src/core/Controller.cpp
    src/core/Miner.cpp
    src/core/Taskbar.cpp
    src/net/JobResults.cpp
    src/net/Network.cpp
    src/net/strategies/DonateStrategy.cpp
    src/Summary.cpp
    src/liquidminer.cpp
   )

set(SOURCES_CRYPTO
    src/crypto/cn/c_blake256.c
    src/crypto/cn/c_groestl.c
    src/crypto/cn/c_jh.c
    src/crypto/cn/c_skein.c
    src/crypto/civiclight/CivicLight.cpp
    src/crypto/civiclight/yespower/yespower-opt.c
    src/crypto/civiclight/yespower/yespower_sha256.c
    src/crypto/cn/CnCtx.cpp
    src/crypto/cn/CnHash.cpp
    src/crypto/common/HugePagesInfo.cpp
    src/crypto/common/MemoryPool.cpp
    src/crypto/common/Nonce.cpp
    src/crypto/common/VirtualMemory.cpp
    src/crypto/nexapow/NexaPow.cpp
   )

if (WITH_VERUSHASH)
    list(APPEND HEADERS_CRYPTO
        thirdparty-bench/algo-sources/verus/src/crypto/haraka.h
        thirdparty-bench/algo-sources/verus/src/crypto/verus_clhash.h
        thirdparty-bench/algo-sources/verus/src/crypto/verus_hash.h
    )
    list(APPEND SOURCES_CRYPTO
        thirdparty-bench/algo-sources/verus/src/crypto/haraka.c
        thirdparty-bench/algo-sources/verus/src/crypto/verus_clhash.c
        thirdparty-bench/algo-sources/verus/src/crypto/verus_clhash_v2.c
        thirdparty-bench/algo-sources/verus/src/crypto/verus_hash.cpp
    )
    if (CMAKE_C_COMPILER_ID MATCHES GNU OR CMAKE_C_COMPILER_ID MATCHES Clang)
        set_source_files_properties(
            thirdparty-bench/algo-sources/verus/src/crypto/verus_clhash.c
            thirdparty-bench/algo-sources/verus/src/crypto/verus_clhash_v2.c
            thirdparty-bench/algo-sources/verus/src/crypto/verus_hash.cpp
            PROPERTIES COMPILE_OPTIONS "-mssse3;-mpclmul;-maes")
    endif()
    add_compile_definitions(XMRIG_ALGO_VERUSHASH)
endif()

if (WITH_XELISHASH)
    list(APPEND HEADERS_CRYPTO src/crypto/xelis/XelisHash.h)
    list(APPEND SOURCES_CRYPTO
        thirdparty-bench/algo-sources/xelis/C/xelis_hash_v3.c
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/chacha20.c
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/chacha20_sse2.c
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/chacha20_avx2.c
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/chacha20_dispatch.c
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/cpu_features.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_portable.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_sse2.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_sse41.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_avx2.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_avx512.c
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_dispatch.c
    )
    include_directories(thirdparty-bench/algo-sources/xelis/C)
    include_directories(thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD)
    include_directories(thirdparty-bench/algo-sources/xelis/C/BLAKE3/c)
    add_compile_definitions(XMRIG_ALGO_XELISHASH)
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/xelis_hash_v3.c
        PROPERTIES COMPILE_DEFINITIONS XELIS_LIQUIDMINER)
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/chacha20_avx2.c
        PROPERTIES COMPILE_OPTIONS "-mavx2")
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/ChaCha20-SIMD/chacha20_sse2.c
        PROPERTIES COMPILE_OPTIONS "-msse2")
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_sse2.c
        PROPERTIES COMPILE_OPTIONS "-msse2")
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_sse41.c
        PROPERTIES COMPILE_OPTIONS "-mssse3;-msse4.1")
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_avx2.c
        PROPERTIES COMPILE_OPTIONS "-mavx2")
    set_source_files_properties(
        thirdparty-bench/algo-sources/xelis/C/BLAKE3/c/blake3_avx512.c
        PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512vl")
endif()

if (CMAKE_C_COMPILER_ID MATCHES GNU)
    set_source_files_properties(src/crypto/cn/CnHash.cpp PROPERTIES COMPILE_FLAGS "-Ofast -fno-tree-vectorize")
endif()

if (WITH_VAES)
    add_definitions(-DXMRIG_VAES)
    set(HEADERS_CRYPTO "${HEADERS_CRYPTO}" src/crypto/cn/CryptoNight_x86_vaes.h)
    set(SOURCES_CRYPTO "${SOURCES_CRYPTO}" src/crypto/cn/CryptoNight_x86_vaes.cpp)
    if (CMAKE_C_COMPILER_ID MATCHES GNU OR CMAKE_C_COMPILER_ID MATCHES Clang)
        set_source_files_properties(src/crypto/cn/CryptoNight_x86_vaes.cpp PROPERTIES COMPILE_FLAGS "-Ofast -fno-tree-vectorize -mavx2 -mvaes")
    endif()
endif()

if (WITH_HWLOC)
    list(APPEND HEADERS_CRYPTO
        src/crypto/common/NUMAMemoryPool.h
        )

    list(APPEND SOURCES_CRYPTO
        src/crypto/common/NUMAMemoryPool.cpp
        src/crypto/common/VirtualMemory_hwloc.cpp
        )
endif()

if (XMRIG_OS_WIN)
    list(APPEND SOURCES_OS
        res/app.rc
        src/App_win.cpp
        src/crypto/common/VirtualMemory_win.cpp
        )

    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv dbghelp)
elseif (XMRIG_OS_APPLE)
    list(APPEND SOURCES_OS
        src/App_unix.cpp
        src/crypto/common/VirtualMemory_unix.cpp
        )

    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORESERVICES_LIBRARY CoreServices)
    set(EXTRA_LIBS ${IOKIT_LIBRARY} ${CORESERVICES_LIBRARY})
else()
    list(APPEND SOURCES_OS
        src/App_unix.cpp
        src/crypto/common/VirtualMemory_unix.cpp
        )

    if (XMRIG_OS_ANDROID)
        set(EXTRA_LIBS pthread rt dl log)
    elseif (XMRIG_OS_LINUX)
        list(APPEND SOURCES_OS
            src/crypto/common/LinuxMemory.h
            src/crypto/common/LinuxMemory.cpp
            )

        set(EXTRA_LIBS pthread rt dl)
    elseif (XMRIG_OS_FREEBSD)
        set(EXTRA_LIBS kvm pthread)
    endif()
endif()

add_definitions(-DXMRIG_MINER_PROJECT -DXMRIG_JSON_SINGLE_LINE_ARRAY)
add_definitions(-D__STDC_FORMAT_MACROS -DUNICODE -D_FILE_OFFSET_BITS=64)

find_package(UV REQUIRED)

include(cmake/flags.cmake)
include(cmake/randomx.cmake)
include(cmake/argon2.cmake)
include(cmake/kawpow.cmake)
include(cmake/ghostrider.cmake)
include(cmake/OpenSSL.cmake)
include(cmake/asm.cmake)

if (WITH_CN_LITE)
    add_definitions(/DXMRIG_ALGO_CN_LITE)
endif()

if (WITH_CN_HEAVY)
    add_definitions(/DXMRIG_ALGO_CN_HEAVY)
endif()

if (WITH_CN_PICO)
    add_definitions(/DXMRIG_ALGO_CN_PICO)
endif()

if (WITH_CN_FEMTO)
    add_definitions(/DXMRIG_ALGO_CN_FEMTO)
endif()

if (WITH_EMBEDDED_CONFIG)
    add_definitions(/DXMRIG_FEATURE_EMBEDDED_CONFIG)
endif()

include(src/hw/api/api.cmake)
include(src/hw/dmi/dmi.cmake)

include_directories(src)
include_directories(src/3rdparty)
if (WITH_VERUSHASH)
    include_directories(thirdparty-bench/algo-sources/verus/src)
    include_directories(thirdparty-bench/algo-sources/verus/src/crypto)
endif()
include_directories(${UV_INCLUDE_DIR})

if (WITH_DEBUG_LOG)
    add_definitions(/DAPP_DEBUG)
endif()

if (WIN32 AND BUILD_STATIC AND WITH_HWLOC AND CMAKE_CXX_COMPILER_ID MATCHES GNU)
    find_library(LTDL_STATIC_LIBRARY
        NAMES libltdl.a
        PATHS "${XMRIG_DEPS}" "$ENV{XMRIG_DEPS}" "$ENV{MINGW_PREFIX}" "C:/tools/msys64/mingw64"
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH
    )
    if (LTDL_STATIC_LIBRARY)
        list(APPEND STATIC_HWLOC_LIBS ${LTDL_STATIC_LIBRARY})
    endif()
endif()

add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES} ${SOURCES_OS} ${HEADERS_CRYPTO} ${SOURCES_CRYPTO} ${SOURCES_SYSLOG} ${TLS_SOURCES} ${XMRIG_ASM_SOURCES})

if (WITH_PEARL_HIP)
    add_dependencies(${CMAKE_PROJECT_NAME} liquidminer_pearl_hip)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PEARL_HIP_DLL}" "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>")
endif()
# Keep the topology library after algorithm archives too: GhostRider and other
# static objects can reference hwloc after its first occurrence on GNU linkers.
target_link_libraries(${CMAKE_PROJECT_NAME} ${XMRIG_ASM_LIBRARY} ${OPENSSL_LIBRARIES} ${UV_LIBRARIES} ${EXTRA_LIBS} ${CPUID_LIB} ${ARGON2_LIBRARY} ${ETHASH_LIBRARY} ${GHOSTRIDER_LIBRARY} ${CPUID_LIB} ${STATIC_HWLOC_LIBS})

if (WITH_NEXA_HIP)
    target_link_libraries(${CMAKE_PROJECT_NAME} "${NEXA_HIP_LIBRARY}" "${NEXA_HIPRTC_LIBRARY}")
    foreach(_hip_runtime amdhip64_7.dll amd_comgr0702.dll amd_comgr_3.dll hiprtc0702.dll hiprtc-builtins0702.dll)
        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "C:/Program Files/AMD/ROCm/7.2/bin/${_hip_runtime}"
                "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${_hip_runtime}")
    endforeach()
endif()

if (WITH_VULKAN_KAWPOW)
    # Rust's GNU staticlib must precede the Win32 system libraries so the
    # MinGW linker can resolve its transitive runtime imports.
    target_link_libraries(${CMAKE_PROJECT_NAME} "${VULKAN_KAWPOW_RUST_LIB}" ws2_32 ntdll bcrypt)
endif()

if (WIN32)
    if (WITH_MSR AND NOT ARM_TARGET)
        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/bin/WinRing0/WinRing0x64.sys" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
    endif()

    if (WITH_BENCHMARK)
        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/benchmark_1M.cmd" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/benchmark_10M.cmd" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
    endif()
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/pool_mine_example.cmd" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/solo_mine_example.cmd" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/scripts/rtm_ghostrider_example.cmd" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES Clang AND CMAKE_BUILD_TYPE STREQUAL Release AND NOT CMAKE_GENERATOR STREQUAL Xcode)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>")
endif()
