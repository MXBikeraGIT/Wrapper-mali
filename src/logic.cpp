#include <vector>
#include <cstdint>
#include <cstring>
#include <android/log.h>
#include <vulkan/vulkan.h>

#define LOG_TAG "WinlatorLogic"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// ============================================================================
// 1. Vulkan Fallback Definitions for DXVK 2.0 Minimum Requirements
// ============================================================================

#ifndef VK_EXT_ROBUSTNESS_2_EXTENSION_NAME
#define VK_EXT_ROBUSTNESS_2_EXTENSION_NAME "VK_EXT_robustness2"
#endif

#ifndef VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME
#define VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME "VK_EXT_transform_feedback"
#endif

#ifndef VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME
#define VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME "VK_EXT_depth_clip_enable"
#endif

#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT static_cast<VkStructureType>(1000286000)
#endif

#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT static_cast<VkStructureType>(1000028002)
#endif

#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT static_cast<VkStructureType>(1000102000)
#endif

// Faked extensions required to pass DXVK 2.0 initialization checks
static const char* FAKED_EXTENSIONS[] = {
    VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
    VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME,
    VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME
};
static const uint32_t FAKED_EXT_COUNT = sizeof(FAKED_EXTENSIONS) / sizeof(FAKED_EXTENSIONS[0]);

// External drivers interface (from output.cpp)
extern VkResult call_real_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
extern VkResult call_real_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
extern VkResult call_real_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
extern VkResult call_real_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
extern void call_real_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures);

// ============================================================================
// 2. SPIR-V Processing Pipeline
// ============================================================================

constexpr uint16_t OP_NOP = 0;
constexpr uint16_t OP_CAPABILITY = 17;
constexpr uint16_t OP_DECORATE = 71;
constexpr uint16_t OP_CONSTANT_COMPOSITE = 43;
constexpr uint16_t OP_SPEC_CONSTANT_COMPOSITE = 51;

constexpr uint32_t CAPABILITY_CLIP_DISTANCE = 32;
constexpr uint32_t DECORATE_BUILTIN = 11;
constexpr uint32_t BUILTIN_CLIP_DISTANCE = 25;

std::vector<uint32_t> process_spirv_shader(const uint32_t* pCode, size_t codeSizeWords) {
    std::vector<uint32_t> spirv(pCode, pCode + codeSizeWords);

    if (spirv.size() < 5 || spirv[0] != 0x07230203) {
        return spirv;
    }

    size_t idx = 5;
    while (idx < spirv.size()) {
        uint32_t instruction = spirv[idx];
        uint16_t opcode = instruction & 0xFFFF;
        uint16_t wordCount = instruction >> 16;

        if (wordCount == 0 || (idx + wordCount) > spirv.size()) {
            break;
        }

        // PASS 1: Remove ClipDistance Capability
        if (opcode == OP_CAPABILITY && wordCount >= 2) {
            uint32_t capability = spirv[idx + 1];
            if (capability == CAPABILITY_CLIP_DISTANCE) {
                for (size_t n = 0; n < wordCount; ++n) {
                    spirv[idx + n] = (OP_NOP & 0xFFFF) | (1 << 16);
                }
            }
        }

        // PASS 2: Fix Spec Composite Constants
        if (opcode == OP_SPEC_CONSTANT_COMPOSITE) {
            spirv[idx] = (wordCount << 16) | OP_CONSTANT_COMPOSITE;
        }

        // PASS 3: Strip BuiltIn ClipDistance Decorators
        if (opcode == OP_DECORATE && wordCount >= 4) {
            uint32_t decoration = spirv[idx + 2];
            uint32_t builtInType = spirv[idx + 3];
            if (decoration == DECORATE_BUILTIN && builtInType == BUILTIN_CLIP_DISTANCE) {
                for (size_t n = 0; n < wordCount; ++n) {
                    spirv[idx + n] = (OP_NOP & 0xFFFF) | (1 << 16);
                }
            }
        }

        idx += wordCount;
    }

    return spirv;
}

// ============================================================================
// 3. Intercepted Vulkan Logic & DXVK 2.0 Extension Faking
// ============================================================================

VkResult logic_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule) 
{
    if (!pCreateInfo || !pCreateInfo->pCode || pCreateInfo->codeSize == 0) {
        return call_real_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    }

    size_t codeSizeWords = pCreateInfo->codeSize / sizeof(uint32_t);
    std::vector<uint32_t> sanitizedCode = process_spirv_shader(pCreateInfo->pCode, codeSizeWords);

    VkShaderModuleCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.pCode = sanitizedCode.data();
    modifiedCreateInfo.codeSize = sanitizedCode.size() * sizeof(uint32_t);

    return call_real_vkCreateShaderModule(device, &modifiedCreateInfo, pAllocator, pShaderModule);
}

VkResult logic_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) 
{
    LOGI(">>> Application requested vkCreateInstance");
    return call_real_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

// Intercepts extension enumeration to report robustness2, transform_feedback, etc.
VkResult logic_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties)
{
    uint32_t realCount = 0;
    VkResult result = call_real_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &realCount, nullptr);
    if (result != VK_SUCCESS) return result;

    if (!pProperties) {
        *pPropertyCount = realCount + FAKED_EXT_COUNT;
        return VK_SUCCESS;
    }

    std::vector<VkExtensionProperties> properties(realCount);
    call_real_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &realCount, properties.data());

    for (uint32_t i = 0; i < FAKED_EXT_COUNT; ++i) {
        bool exists = false;
        for (const auto& prop : properties) {
            if (strcmp(prop.extensionName, FAKED_EXTENSIONS[i]) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            VkExtensionProperties fakedExt{};
            strncpy(fakedExt.extensionName, FAKED_EXTENSIONS[i], VK_MAX_EXTENSION_NAME_SIZE - 1);
            fakedExt.specVersion = 1;
            properties.push_back(fakedExt);
            LOGI("Faking device extension: %s", FAKED_EXTENSIONS[i]);
        }
    }

    uint32_t toCopy = std::min(*pPropertyCount, static_cast<uint32_t>(properties.size()));
    std::memcpy(pProperties, properties.data(), toCopy * sizeof(VkExtensionProperties));
    *pPropertyCount = toCopy;

    return (toCopy < properties.size()) ? VK_INCOMPLETE : VK_SUCCESS;
}

// Intercepts feature queries to set nullDescriptor & robustBufferAccess2 to VK_TRUE
void logic_vkGetPhysicalDeviceFeatures2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2* pFeatures)
{
    call_real_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);

    if (!pFeatures) return;

    void* currentHeader = pFeatures->pNext;
    while (currentHeader) {
        auto* header = static_cast<VkBaseOutStructure*>(currentHeader);

        if (header->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT) {
            auto* rob2 = reinterpret_cast<VkPhysicalDeviceRobustness2FeaturesEXT*>(header);
            rob2->nullDescriptor = VK_TRUE;
            rob2->robustBufferAccess2 = VK_TRUE;
            rob2->robustImageAccess2 = VK_TRUE;
            LOGI("Satisfied DXVK 2.0 requirement: Faked VK_EXT_robustness2 features (nullDescriptor=1, robustBufferAccess2=1)");
        }
        else if (header->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT) {
            auto* tf = reinterpret_cast<VkPhysicalDeviceTransformFeedbackFeaturesEXT*>(header);
            tf->transformFeedback = VK_TRUE;
            tf->geometryStreams = VK_TRUE;
        }
        else if (header->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT) {
            auto* dc = reinterpret_cast<VkPhysicalDeviceDepthClipEnableFeaturesEXT*>(header);
            dc->depthClipEnable = VK_TRUE;
        }

        currentHeader = header->pNext;
    }
}

// Strips faked extension names and pNext structs before passing to native driver
VkResult logic_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    if (!pCreateInfo) {
        return call_real_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    }

    // 1. Filter enabled extension names
    std::vector<const char*> filteredExtensions;
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
        const char* extName = pCreateInfo->ppEnabledExtensionNames[i];
        bool isFaked = false;
        for (uint32_t j = 0; j < FAKED_EXT_COUNT; ++j) {
            if (strcmp(extName, FAKED_EXTENSIONS[j]) == 0) {
                isFaked = true;
                LOGI("Stripping faked extension from vkCreateDevice: %s", extName);
                break;
            }
        }
        if (!isFaked) {
            filteredExtensions.push_back(extName);
        }
    }

    // 2. Filter pNext chain feature structs
    VkBaseOutStructure* prev = nullptr;
    VkBaseOutStructure* head = const_cast<VkBaseOutStructure*>(reinterpret_cast<const VkBaseOutStructure*>(pCreateInfo->pNext));
    VkBaseOutStructure* curr = head;

    while (curr) {
        if (curr->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT ||
            curr->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT ||
            curr->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT) {
            LOGI("Stripping faked feature struct from vkCreateDevice pNext (sType: %d)", curr->sType);
            if (prev) {
                prev->pNext = curr->pNext;
            } else {
                head = reinterpret_cast<VkBaseOutStructure*>(curr->pNext);
            }
        } else {
            prev = curr;
        }
        curr = reinterpret_cast<VkBaseOutStructure*>(curr->pNext);
    }

    VkDeviceCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.enabledExtensionCount = static_cast<uint32_t>(filteredExtensions.size());
    modifiedCreateInfo.ppEnabledExtensionNames = filteredExtensions.data();
    modifiedCreateInfo.pNext = head;

    return call_real_vkCreateDevice(physicalDevice, &modifiedCreateInfo, pAllocator, pDevice);
}
