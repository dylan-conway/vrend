#include "vrend.h"

// Need to cast to uint32_t when used
struct QueueFamilies {
    int graphics_queue_index;
    int present_queue_index;
};

struct PhysicalDeviceDetails {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures features;

    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    uint32_t num_formats;
    VkPresentModeKHR* present_modes;
    uint32_t num_present_modes;
};

VkBool32 _CheckInstanceExtensions();
VkBool32 _CheckPhysicalDeviceExtensions(VkPhysicalDevice physical_device);
struct QueueFamilies _GetQueueFamilies(VkPhysicalDevice physical_device);
VkBool32 _CheckPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device);
void _SetCurrentPhysicalDevice(VkPhysicalDevice physical_device);

#define NUM_NEEDED_PHYSICAL_DEVICE_EXTENSIONS 1
static const char* _needed_physical_device_extensions[NUM_NEEDED_PHYSICAL_DEVICE_EXTENSIONS] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static SDL_Window* _sdl_window = NULL;
static VkInstance _vk_instance = NULL;
static VkSurfaceKHR _vk_surface = NULL;

static VkPhysicalDevice* _physical_devices = NULL;
static struct PhysicalDeviceDetails _current_physical_device = {0};

/**
 * Vulkan renderer initialization steps:
 * 1    - Initialize SDL2 and create window
 * 2    - Check instance extensions and layers (only check
 *        layers if debug is enabled)
 * 3    - Create instance with app info and instance create info
 * 4    - Initialize debug utils if enabled
 * 5    - Create Vulkan surface with SDL2
 * 6    - Find Vulkan capable physical devices and select one to use
 * 7    - 
 */

void INIT_VREND(char* title, uint32_t w, uint32_t h){

    // -----    1   -----
    SDL_Init(SDL_INIT_EVERYTHING);
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
    _sdl_window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, flags
    );
    if(_sdl_window == NULL){
        fprintf(stderr, "ERROR: failed to create SDL2 window\n");
        exit(EXIT_FAILURE);
    }

    // -----    2   -----
    #if defined DEBUG
        if(!CheckInstanceLayers()){
            fprintf(stderr, "ERROR: Vulkan instance does not support required layers\n");
            exit(EXIT_FAILURE);
        }
    #endif
    if(!_CheckInstanceExtensions()){
        fprintf(stderr, "ERROR: Vulkan instance does not support required extensions\n");
        exit(EXIT_FAILURE);
    }

    // -----    3   -----
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = title;
    app_info.pEngineName = "No engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;

    uint32_t num_instance_extensions = 0;
    SDL_Vulkan_GetInstanceExtensions(_sdl_window, &num_instance_extensions, NULL);
    const char** instance_extensions = malloc(sizeof(char*) * num_instance_extensions);
    SDL_Vulkan_GetInstanceExtensions(_sdl_window, &num_instance_extensions, instance_extensions);
    #if defined DEBUG
        num_instance_extensions += 1;
        instance_extensions = realloc(instance_extensions, sizeof(char*) * num_instance_extensions);
        instance_extensions[num_instance_extensions - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    #endif

    VkInstanceCreateInfo instance_ci = {0};
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pApplicationInfo = &app_info;
    instance_ci.enabledExtensionCount = num_instance_extensions;
    instance_ci.ppEnabledExtensionNames = instance_extensions;
    #if defined DEBUG
        DebugUtilsInstanceLayers(&instance_ci);
    #else
        instance_ci.enabledLayerCount = 0;
        instance_ci.ppEnabledLayerNames = NULL;
        instance_ci.pNext = NULL;
    #endif

    VkResult result;
    result = vkCreateInstance(&instance_ci, NULL, &_vk_instance);
    if(result != VK_SUCCESS){
        fprintf(stderr, "VK ERROR %d: failed to create Vulkan instance\n", result);
        exit(EXIT_FAILURE);
    }

    // -----    4   -----
    #if defined DEBUG
        InitDebugUtils(&_vk_instance);
    #endif

    // -----    5   -----
    if(SDL_Vulkan_CreateSurface(_sdl_window, _vk_instance, &_vk_surface) == SDL_FALSE){
        fprintf(stderr, "ERROR: failed to create Vulkan surface with SDL2\n");
        exit(EXIT_FAILURE);
    }

    // -----    6   -----
    // (Chooses the first suitable physical device)
    uint32_t num_physical_devices = 0;
    vkEnumeratePhysicalDevices(_vk_instance, &num_physical_devices, NULL);
    if(num_physical_devices == 0){
        fprintf(stderr, "ERROR: failed to find a device with Vulkan support\n");
        exit(EXIT_FAILURE);
    }
    _physical_devices = malloc(sizeof(VkPhysicalDevice) * num_physical_devices);
    vkEnumeratePhysicalDevices(_vk_instance, &num_physical_devices, _physical_devices);
    for(uint32_t i = 0; i < num_physical_devices; i ++){
        if(_physical_devices != NULL){
            if(_CheckPhysicalDeviceExtensions(_physical_devices[i]) == VK_FALSE){
                _physical_devices[i] = NULL;
            }
        }
    }
    for(uint32_t i = 0; i < num_physical_devices; i ++){
        if(_physical_devices != NULL){
            struct QueueFamilies qfamilies = _GetQueueFamilies(_physical_devices[i]);
            if(qfamilies.graphics_queue_index == -1 || qfamilies.present_queue_index == -1){
                _physical_devices[i] = NULL;
            }
        }
    }
    for(uint32_t i = 0; i < num_physical_devices; i ++){
        if(_physical_devices != NULL){
            if(_CheckPhysicalDeviceSurfaceCapabilities(_physical_devices[i]) == VK_FALSE){
                _physical_devices[i] = NULL;
            }
        }
    }
    _SetCurrentPhysicalDevice(_physical_devices[0]);

    // -----    7   -----
}

void FREE_VREND(){
    #if defined DEBUG
        FreeDebugUtils(&_vk_instance);
    #endif
    vkDestroySurfaceKHR(_vk_instance, _vk_surface, NULL);
    vkDestroyInstance(_vk_instance, NULL);
    SDL_DestroyWindow(_sdl_window);
    SDL_Quit();
}

VkBool32 _CheckInstanceExtensions(){

    // Get the needed extensions for the platform (SDL2)
    uint32_t num_needed_extensions = 0;
    SDL_Vulkan_GetInstanceExtensions(_sdl_window, &num_needed_extensions, NULL);
    const char** needed_extensions = malloc(sizeof(char*) * num_needed_extensions);
    SDL_Vulkan_GetInstanceExtensions(_sdl_window, &num_needed_extensions, needed_extensions);

    // Get the available extensions from system
    uint32_t num_extensions = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL);
    VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * num_extensions);
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions);

    // Determine if the needed extensions are available
    VkBool32 has_required = VK_TRUE;
    for(uint32_t i = 0; i < num_needed_extensions; i ++){

        VkBool32 found_extension = VK_FALSE;
        for(uint32_t j = 0; j < num_extensions; j ++){
            if(strcmp(needed_extensions[i], extensions[j].extensionName) == 0){
                found_extension = VK_TRUE;
                break;
            }
        }

        if(found_extension == VK_FALSE){
            has_required = VK_FALSE;
            break;
        }
    }

    free(needed_extensions);
    free(extensions);
    return has_required;
}

VkBool32 _CheckPhysicalDeviceExtensions(VkPhysicalDevice physical_device){
    
    // Get the extensions available on the physical device
    uint32_t num_extensions = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_extensions, NULL);
    VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * num_extensions);
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_extensions, extensions);

    // Determine if the physical device has the needed physical device extensions
    VkBool32 has_required = VK_TRUE;
    for(uint32_t i = 0; i < NUM_NEEDED_PHYSICAL_DEVICE_EXTENSIONS; i ++){

        VkBool32 found_extension = VK_FALSE;
        for(uint32_t j = 0; j < num_extensions; j ++){
            if(strcmp(_needed_physical_device_extensions[i], extensions[j].extensionName) == 0){
                found_extension = VK_TRUE;
                break;
            }
        }

        if(found_extension == VK_FALSE){
            has_required = VK_FALSE;
            break;
        }
    }

    free(extensions);
    return has_required;
}

struct QueueFamilies _GetQueueFamilies(VkPhysicalDevice physical_device){

    struct QueueFamilies qfamilies = {
        .graphics_queue_index = -1,
        .present_queue_index = -1
    };

    // Find the number of queue familes the physical device has
    uint32_t num_queues = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queues, NULL);
    VkQueueFamilyProperties* queue_properties = malloc(sizeof(VkQueueFamilyProperties) * num_queues);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queues, queue_properties);

    // Record the available queue families into the struct
    for(uint32_t i = 0; i < num_queues; i ++){
        if((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT){
            qfamilies.graphics_queue_index = i;
        }
        VkBool32 present_family = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, _vk_surface, &present_family);
        if(present_family == VK_TRUE){
            qfamilies.present_queue_index = i;
        }
    }

    free(queue_properties);
    return qfamilies;
}

VkBool32 _CheckPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device){

    VkBool32 has_required_capabilities = VK_FALSE;

    // VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    // vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, _vk_surface, &surface_capabilities);

    // Check if there are surface formats
    uint32_t num_formats = 0;
    VkSurfaceFormatKHR* formats = NULL;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, _vk_surface, &num_formats, NULL);
    if(num_formats == 0){
        return VK_FALSE;
    }

    // Check if there are surface present modes
    uint32_t num_present_modes = 0;
    VkPresentModeKHR* present_modes = NULL;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, _vk_surface, &num_present_modes, NULL);
    if(num_present_modes == 0){
        return VK_FALSE;
    }

    return VK_TRUE;
}

void _SetCurrentPhysicalDevice(VkPhysicalDevice physical_device){

    // Properties, memory properties, and features
    _current_physical_device.device = physical_device;
    vkGetPhysicalDeviceProperties(physical_device, &_current_physical_device.properties);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &_current_physical_device.memory_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &_current_physical_device.features);

    // Capabilities, formats, and present modes
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, _vk_surface, &_current_physical_device.capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, _vk_surface, &_current_physical_device.num_formats, NULL);
    free(_current_physical_device.formats);
    _current_physical_device.formats = malloc(sizeof(VkSurfaceFormatKHR) * _current_physical_device.num_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, _vk_surface,
        &_current_physical_device.num_formats,
        _current_physical_device.formats
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, _vk_surface, &_current_physical_device.num_present_modes, NULL);
    free(_current_physical_device.present_modes);
    _current_physical_device.present_modes = malloc(sizeof(VkPresentModeKHR) * _current_physical_device.num_present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, _vk_surface,
        &_current_physical_device.num_present_modes,
        _current_physical_device.present_modes
    );
}