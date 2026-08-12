#include <vulkan/vulkan.h>
#include <android/log.h>
#include <cstring>
#include <vector>

#define LOG_TAG "WinlatorBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// External references from logic.cpp
extern std::vector<uint32_t> process_spirv_shader(const uint32_t* pCode, size_t codeSizeWords);

// External references from output.cpp
extern void init_android_vulkan();
extern PFN_vkVoidFunction get_real_proc_addr(VkInstance instance, const char* pName);
extern VkResult call_real_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
extern VkResult call_real_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);

// Intercepted: vkCreateShaderModule
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule) 
{
    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize == 0) {
        return call_real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    size_t codeSizeWords = pCreateInfo->codeSize / sizeof(uint32_t);

    // Pass raw SPIR-V to logic.cpp for Mali sanitization
    std::vector<uint32_t> sanitizedCode = process_spirv_shader(pCreateInfo->pCode, codeSizeWords);

    // Rebind new SPIR-V payload
    VkShaderModuleCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.pCode = sanitizedCode.data();
    modifiedCreateInfo.codeSize = sanitizedCode.size() * sizeof(uint32_t);

    LOGI("Shader intercepted & rewritten! Original: %zud words -> New: %zud words", 
         codeSizeWords, sanitizedCode.size());

    // Dispatch modified shader to Android system GPU driver
    return call_real_vkCreateShaderModule(device, &modifiedCreateInfo, pAllocator, pShaderModule);
}

// Intercepted: vkCreateInstance
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) 
{
    LOGI(">>> Winlator called vkCreateInstance! Application: %s", 
         (pCreateInfo && pCreateInfo->pApplicationInfo && pCreateInfo->pApplicationInfo->pApplicationName) 
         ? pCreateInfo->pApplicationInfo->pApplicationName : "DXVK Game");

    return call_real_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

// Main Hook Entry Point exposed to Winlator
extern "C" {

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    init_android_vulkan();

    if (!pName) return nullptr;

    // Route functions we want to modify to our local hooks
    if (strcmp(pName, "vkCreateShaderModule") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(Hook_vkCreateShaderModule);
    }
    if (strcmp(pName, "vkCreateInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(Hook_vkCreateInstance);
    }

    // Direct passthrough for all unmodified Vulkan calls
    return get_real_proc_addr(instance, pName);
}

// Android ICD entrypoint fallback
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return vkGetInstanceProcAddr(instance, pName);
}

} // extern "C"
