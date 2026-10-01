if (BUILD_STATIC AND XMRIG_OS_UNIX AND WITH_OPENCL)
    message(WARNING "OpenCL backend is not compatible with static build, use -DWITH_OPENCL=OFF to suppress this warning")

    set(WITH_OPENCL OFF)
endif()

if (WITH_OPENCL)
    add_definitions(/DXMRIG_FEATURE_OPENCL /DCL_USE_DEPRECATED_OPENCL_1_2_APIS)

    # OggPoW is a ProgPoW variant.  Its chain-specific kernel and generated
    # program are kept as a build dependency so the OpenCL backend uses the
    # same work format as the reference Ogg miner.
    set(OGGPOW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/thirdparty-bench/algo-sources/oggpow-miner")
    set(OGGPOW_KERNEL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/oggpow_cl.h")
    set(XELIS_KERNEL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xelishash_v3_cl.h")
    set(NEXAPOW_KERNEL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/nexapow_sha_cl.h")
    set(NEXAPOW_HIP_KERNEL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/nexapow_hip_cl.h")
    set(NEXAPOW_ECC_HEADER "${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexa/inc_ecc_secp256k1.h")
    set(NEXAPOW_ECC_SOURCE "${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexa/inc_ecc_secp256k1.cl")
    add_custom_command(
        OUTPUT ${OGGPOW_KERNEL_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DTXT2STR_SOURCE_FILE=${OGGPOW_SOURCE_DIR}/libethash-cl/CLMiner_kernel.cl
            -DTXT2STR_VARIABLE_NAME=oggpow_cl
            -DTXT2STR_HEADER_FILE=${OGGPOW_KERNEL_HEADER}
            -P ${OGGPOW_SOURCE_DIR}/cmake/txt2str.cmake
        DEPENDS ${OGGPOW_SOURCE_DIR}/libethash-cl/CLMiner_kernel.cl
                ${OGGPOW_SOURCE_DIR}/cmake/txt2str.cmake
        VERBATIM)
    add_custom_command(
        OUTPUT ${NEXAPOW_HIP_KERNEL_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DTXT2STR_SOURCE_FILE=${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexapow/nexapow_sha.cl
            -DTXT2STR_ECC_HEADER=${NEXAPOW_ECC_HEADER}
            -DTXT2STR_ECC_SOURCE=${NEXAPOW_ECC_SOURCE}
            -DTXT2STR_VARIABLE_NAME=nexapow_hip_cl
            -DTXT2STR_HEADER_FILE=${NEXAPOW_HIP_KERNEL_HEADER}
            -P ${CMAKE_SOURCE_DIR}/cmake/scripts/nexapow_hip_txt2str.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexapow/nexapow_sha.cl
                ${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexa/nexapow_g_table.cl
                ${NEXAPOW_ECC_HEADER} ${NEXAPOW_ECC_SOURCE}
                ${CMAKE_SOURCE_DIR}/cmake/scripts/nexapow_hip_txt2str.cmake
        VERBATIM)
    add_custom_command(
        OUTPUT ${XELIS_KERNEL_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DTXT2STR_SOURCE_FILE=${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/xelis/xelishash_v3.cl
            -DTXT2STR_VARIABLE_NAME=xelishash_v3_cl
            -DTXT2STR_HEADER_FILE=${XELIS_KERNEL_HEADER}
            -P ${OGGPOW_SOURCE_DIR}/cmake/txt2str.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/xelis/xelishash_v3.cl
                ${OGGPOW_SOURCE_DIR}/cmake/txt2str.cmake
        VERBATIM)
    add_custom_command(
        OUTPUT ${NEXAPOW_KERNEL_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DTXT2STR_SOURCE_FILE=${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexapow/nexapow_sha.cl
            -DTXT2STR_ECC_HEADER=${NEXAPOW_ECC_HEADER}
            -DTXT2STR_ECC_SOURCE=${NEXAPOW_ECC_SOURCE}
            -DTXT2STR_VARIABLE_NAME=nexapow_sha_cl
            -DTXT2STR_HEADER_FILE=${NEXAPOW_KERNEL_HEADER}
            -P ${CMAKE_SOURCE_DIR}/cmake/scripts/nexapow_txt2str.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexapow/nexapow_sha.cl
                ${CMAKE_SOURCE_DIR}/src/backend/opencl/cl/nexa/nexapow_g_table.cl
                ${NEXAPOW_ECC_HEADER} ${NEXAPOW_ECC_SOURCE}
                ${CMAKE_SOURCE_DIR}/cmake/scripts/nexapow_txt2str.cmake
        VERBATIM)

    set(HEADERS_BACKEND_OPENCL
        src/backend/opencl/cl/OclSource.h
        src/backend/opencl/interfaces/IOclRunner.h
        src/backend/opencl/OclBackend.h
        src/backend/opencl/OclCache.h
        src/backend/opencl/OclConfig.h
        src/backend/opencl/OclConfig_gen.h
        src/backend/opencl/OclGenerator.h
        src/backend/opencl/OclLaunchData.h
        src/backend/opencl/OclThread.h
        src/backend/opencl/OclThreads.h
        src/backend/opencl/OclWorker.h
        src/backend/opencl/runners/OclBaseRunner.h
        src/backend/opencl/runners/OclOggPowRunner.h
        src/backend/opencl/runners/OclXelisHashRunner.h
        src/backend/opencl/runners/OclNexaPowRunner.h
        ${OGGPOW_KERNEL_HEADER}
        ${XELIS_KERNEL_HEADER}
        ${NEXAPOW_KERNEL_HEADER}
        ${NEXAPOW_HIP_KERNEL_HEADER}
        src/backend/opencl/runners/tools/OclSharedData.h
        src/backend/opencl/runners/tools/OclSharedState.h
        src/backend/opencl/wrappers/OclContext.h
        src/backend/opencl/wrappers/OclDevice.h
        src/backend/opencl/wrappers/OclError.h
        src/backend/opencl/wrappers/OclKernel.h
        src/backend/opencl/wrappers/OclLib.h
        src/backend/opencl/wrappers/OclPlatform.h
        src/backend/opencl/wrappers/OclVendor.h
        )

    set(SOURCES_BACKEND_OPENCL
        src/backend/opencl/cl/OclSource.cpp
        src/backend/opencl/OclBackend.cpp
        src/backend/opencl/OclCache.cpp
        src/backend/opencl/OclConfig.cpp
        src/backend/opencl/OclLaunchData.cpp
        src/backend/opencl/OclThread.cpp
        src/backend/opencl/OclThreads.cpp
        src/backend/opencl/OclWorker.cpp
        src/backend/opencl/generators/ocl_generic_gpupow_generator.cpp
        src/backend/opencl/runners/OclBaseRunner.cpp
        src/backend/opencl/runners/OclOggPowRunner.cpp
        src/backend/opencl/runners/OclXelisHashRunner.cpp
        src/backend/opencl/runners/OclNexaPowRunner.cpp
        ${OGGPOW_SOURCE_DIR}/libprogpow/ProgPow.cpp
        src/backend/opencl/runners/tools/OclSharedData.cpp
        src/backend/opencl/runners/tools/OclSharedState.cpp
        src/backend/opencl/wrappers/OclContext.cpp
        src/backend/opencl/wrappers/OclDevice.cpp
        src/backend/opencl/wrappers/OclError.cpp
        src/backend/opencl/wrappers/OclKernel.cpp
        src/backend/opencl/wrappers/OclLib.cpp
        src/backend/opencl/wrappers/OclPlatform.cpp
        )

    include_directories(${OGGPOW_SOURCE_DIR}/libprogpow ${CMAKE_CURRENT_BINARY_DIR})

    if (XMRIG_OS_APPLE)
        add_definitions(/DCL_TARGET_OPENCL_VERSION=120)
        list(APPEND SOURCES_BACKEND_OPENCL src/backend/opencl/wrappers/OclDevice_mac.cpp)
    elseif (WITH_OPENCL_VERSION)
        add_definitions(/DCL_TARGET_OPENCL_VERSION=${WITH_OPENCL_VERSION})
    endif()

    if (WIN32)
        list(APPEND SOURCES_BACKEND_OPENCL src/backend/opencl/OclCache_win.cpp)
    else()
        list(APPEND SOURCES_BACKEND_OPENCL src/backend/opencl/OclCache_unix.cpp)
    endif()

    if (WITH_RANDOMX)
        list(APPEND HEADERS_BACKEND_OPENCL
             src/backend/opencl/kernels/rx/Blake2bHashRegistersKernel.h
             src/backend/opencl/kernels/rx/Blake2bInitialHashBigKernel.h
             src/backend/opencl/kernels/rx/Blake2bInitialHashDoubleKernel.h
             src/backend/opencl/kernels/rx/Blake2bInitialHashKernel.h
             src/backend/opencl/kernels/rx/ExecuteVmKernel.h
             src/backend/opencl/kernels/rx/FillAesKernel.h
             src/backend/opencl/kernels/rx/FindSharesKernel.h
             src/backend/opencl/kernels/rx/HashAesKernel.cpp
             src/backend/opencl/kernels/rx/InitVmKernel.h
             src/backend/opencl/kernels/rx/RxJitKernel.h
             src/backend/opencl/kernels/rx/RxRunKernel.h
             src/backend/opencl/runners/OclRxBaseRunner.h
             src/backend/opencl/runners/OclRxJitRunner.h
             src/backend/opencl/runners/OclRxVmRunner.h
             )

        list(APPEND SOURCES_BACKEND_OPENCL
             src/backend/opencl/generators/ocl_generic_rx_generator.cpp
             src/backend/opencl/kernels/rx/Blake2bHashRegistersKernel.cpp
             src/backend/opencl/kernels/rx/Blake2bInitialHashBigKernel.cpp
             src/backend/opencl/kernels/rx/Blake2bInitialHashDoubleKernel.cpp
             src/backend/opencl/kernels/rx/Blake2bInitialHashKernel.cpp
             src/backend/opencl/kernels/rx/ExecuteVmKernel.cpp
             src/backend/opencl/kernels/rx/FillAesKernel.cpp
             src/backend/opencl/kernels/rx/FindSharesKernel.cpp
             src/backend/opencl/kernels/rx/HashAesKernel.cpp
             src/backend/opencl/kernels/rx/InitVmKernel.cpp
             src/backend/opencl/kernels/rx/RxJitKernel.cpp
             src/backend/opencl/kernels/rx/RxRunKernel.cpp
             src/backend/opencl/runners/OclRxBaseRunner.cpp
             src/backend/opencl/runners/OclRxJitRunner.cpp
             src/backend/opencl/runners/OclRxVmRunner.cpp
             )
    endif()

    if (WITH_KAWPOW)
        list(APPEND HEADERS_BACKEND_OPENCL
             src/backend/opencl/kernels/kawpow/KawPow_CalculateDAGKernel.h
             src/backend/opencl/runners/OclKawPowRunner.h
             src/backend/opencl/runners/tools/OclKawPow.h
             )

        list(APPEND SOURCES_BACKEND_OPENCL
             src/backend/opencl/generators/ocl_generic_kawpow_generator.cpp
             src/backend/opencl/kernels/kawpow/KawPow_CalculateDAGKernel.cpp
             src/backend/opencl/runners/OclKawPowRunner.cpp
             src/backend/opencl/runners/tools/OclKawPow.cpp
             )
    endif()

    if (WITH_STRICT_CACHE)
        add_definitions(/DXMRIG_STRICT_OPENCL_CACHE)
    else()
        remove_definitions(/DXMRIG_STRICT_OPENCL_CACHE)
    endif()

    if (WITH_INTERLEAVE_DEBUG_LOG)
        add_definitions(/DXMRIG_INTERLEAVE_DEBUG)
    endif()

    if (WITH_ADL AND (XMRIG_OS_WIN OR XMRIG_OS_LINUX))
        add_definitions(/DXMRIG_FEATURE_ADL)

        list(APPEND HEADERS_BACKEND_OPENCL
             src/backend/opencl/wrappers/AdlHealth.h
             src/backend/opencl/wrappers/AdlLib.h
             )

        if (XMRIG_OS_WIN)
            list(APPEND SOURCES_BACKEND_OPENCL src/backend/opencl/wrappers/AdlLib.cpp)
        else()
            list(APPEND SOURCES_BACKEND_OPENCL src/backend/opencl/wrappers/AdlLib_linux.cpp)
        endif()
    else()
       remove_definitions(/DXMRIG_FEATURE_ADL)
    endif()
else()
    remove_definitions(/DXMRIG_FEATURE_OPENCL)
    remove_definitions(/DXMRIG_FEATURE_ADL)

    set(HEADERS_BACKEND_OPENCL "")
    set(SOURCES_BACKEND_OPENCL "")
endif()
