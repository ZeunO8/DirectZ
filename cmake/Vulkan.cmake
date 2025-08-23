if(ANDROID)
    set(VULKAN_LIB_PATH "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/${ANDROID_TRIPLET}/${ANDROID_PLATFORM_LEVEL}/libvulkan.so")
    set(VULKAN_INCLUDE_PATH "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include")
    add_library(Vulkan::Vulkan SHARED IMPORTED GLOBAL)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${VULKAN_LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_PATH}"
    )
    set(Vulkan_FOUND TRUE)
elseif(IOS)
	find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
else()
	find_package(Vulkan REQUIRED)
endif()

message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")