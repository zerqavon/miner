if (WITH_VULKAN_KAWPOW)
    add_definitions(/DXMRIG_FEATURE_VULKAN_KAWPOW)

    set(HEADERS_BACKEND_VULKAN
        src/backend/vulkan/VulkanKawPowApi.h
        src/backend/vulkan/VulkanKawPowRunner.h
        )

    set(SOURCES_BACKEND_VULKAN
        src/backend/vulkan/VulkanKawPowRunner.cpp
        )
else()
    remove_definitions(/DXMRIG_FEATURE_VULKAN_KAWPOW)
    set(HEADERS_BACKEND_VULKAN "")
    set(SOURCES_BACKEND_VULKAN "")
endif()
