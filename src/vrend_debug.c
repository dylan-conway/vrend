#include "vrend_debug.h"

#define NUM_NEEDED_LAYERS 1
static const char* _needed_layers[NUM_NEEDED_LAYERS] = {
    "VK_LAYER_KHRONOS_validation"
};

static VkDebugUtilsMessengerEXT _debug_messenger = NULL;
static VkDebugUtilsMessengerCreateInfoEXT _debug_messenger_ci = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext = NULL,
    .flags = 0,
    .messageSeverity =
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = _DebugCallback,
    .pUserData = NULL
};

void DebugUtilsInstanceLayers(VkInstanceCreateInfo* ci){
    ci->enabledLayerCount = NUM_NEEDED_LAYERS;
    ci->ppEnabledLayerNames = _needed_layers;
    ci->pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&_debug_messenger_ci;
}

VkBool32 CheckInstanceLayers(){

    uint32_t num_layers = 0;
    vkEnumerateInstanceLayerProperties(&num_layers, NULL);
    VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * num_layers);
    vkEnumerateInstanceLayerProperties(&num_layers, layers);

    VkBool32 has_required = VK_TRUE;
    for(uint32_t i = 0; i < NUM_NEEDED_LAYERS; i ++){

        VkBool32 found_layer = VK_FALSE;
        for(uint32_t j = 0; j < num_layers; j ++){
            if(strcmp(_needed_layers[i], layers[j].layerName) == 0){
                found_layer = VK_TRUE;
                break;
            }
        }

        if(found_layer == VK_FALSE){
            has_required = VK_FALSE;
            break;
        }
    }

    free(layers);
    return has_required;
}

void InitDebugUtils(VkInstance* vk_instance){

    _CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        *vk_instance, "vkCreateDebugUtilsMessengerEXT"
    );
    _DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        *vk_instance, "vkDestroyDebugUtilsMessengerEXT"
    );

    VkResult result;
    result = _CreateDebugUtilsMessengerEXT(*vk_instance, &_debug_messenger_ci, NULL, &_debug_messenger);
    if(result != VK_SUCCESS){
        fprintf(stderr, "VK ERROR %d: failed to create debug utils messenger\n", result);
        exit(EXIT_FAILURE);
    }
}

void FreeDebugUtils(VkInstance* vk_instance){
    _DestroyDebugUtilsMessengerEXT(*vk_instance, _debug_messenger, NULL);
}

VKAPI_ATTR VkBool32 VKAPI_CALL _DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
){
    switch(messageSeverity){
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            printf("\e[0;33m%s\n\e[0m", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            printf("\e[0;33m%s\n\e[0m", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            printf("\e[0;35m%s\n\e[0m", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            printf("\e[0;31m%s\n\e[0m", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            printf("\e[0;30m%s\n\e[0m", pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}