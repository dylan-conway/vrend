#include "vrend.h"

// Need to cast to uint32_t when used
struct QueueFamilies {
    int graphics_queue_index;
    int present_queue_index;
    int num_queues;
};

struct PhysicalDeviceDetails {
    VkPhysicalDevice handle;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures features;

    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    uint32_t num_formats;
    VkPresentModeKHR* present_modes;
    uint32_t num_present_modes;

    struct QueueFamilies qfamilies;
};

struct SwapChainDetails {
    VkSwapchainKHR handle;
    VkImage* images;
    VkImageView* image_views;
    uint32_t num_images;
    VkFormat image_format;
    VkExtent2D extent;    
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
static VkDevice _vk_device = NULL;

static VkPhysicalDevice* _physical_devices = NULL;
static struct PhysicalDeviceDetails _pdevice = {0};

static VkQueue _graphics_queue = NULL;
static VkQueue _present_queue = NULL;

static struct SwapChainDetails _swap_chain = {0};

/**
 * Vulkan renderer initialization steps:
 * 1    - Initialize SDL2 and create window.
 * 2    - Check instance extensions and layers (only check
 *        layers if debug is enabled).
 * 3    - Create instance with app info and instace create info.
 * 4    - Initialize debug utils if enabled.
 * 5    - Create Vulkan surface with SDL2.
 * 6    - Find Vulkan capable physical devices and select one to use.
 * 7    - Create the logical device from the selected
 *        physical device.
 * 8    - Get the graphics and present queues after device creation.
 * 9    - Swap chain creation.
 * 10   - Create render pass.
 */

void INIT_VREND(char* title, uint32_t w, uint32_t h){

    {   // -----    1   -----
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
    }


    {   // -----    2   -----
        #ifdef DEBUG
            if(!CheckInstanceLayers()){
                fprintf(stderr, "ERROR: Vulkan instance does not support required layers\n");
                exit(EXIT_FAILURE);
            }
        #endif
        if(!_CheckInstanceExtensions()){
            fprintf(stderr, "ERROR: Vulkan instance does not support required extensions\n");
            exit(EXIT_FAILURE);
        }
    }


    {   // -----    3   -----
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
        #ifdef DEBUG
            num_instance_extensions += 1;
            instance_extensions = realloc(instance_extensions, sizeof(char*) * num_instance_extensions);
            instance_extensions[num_instance_extensions - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        #endif

        VkInstanceCreateInfo instance_ci = {0};
        instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_ci.pApplicationInfo = &app_info;
        instance_ci.enabledExtensionCount = num_instance_extensions;
        instance_ci.ppEnabledExtensionNames = instance_extensions;
        #ifdef DEBUG
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
    }


    {   // -----    4   -----
        #ifdef DEBUG
            InitDebugUtils(&_vk_instance);
        #endif
    }


    {   // -----    5   -----
        SDL_bool result;
        result = SDL_Vulkan_CreateSurface(_sdl_window, _vk_instance, &_vk_surface);
        if(result == SDL_FALSE){
            fprintf(stderr, "ERROR: failed to create Vulkan surface with SDL2\n");
            exit(EXIT_FAILURE);
        }
    }


    {   // -----    6   -----
        // (Chooses first available discrete gpu)
        uint32_t num_physical_devices = 0;
        vkEnumeratePhysicalDevices(_vk_instance, &num_physical_devices, NULL);
        if(num_physical_devices == 0){
            fprintf(stderr, "ERROR: failed to find a device with Vulkan support\n");
            exit(EXIT_FAILURE);
        }
        _physical_devices = malloc(sizeof(VkPhysicalDevice) * num_physical_devices);
        vkEnumeratePhysicalDevices(_vk_instance, &num_physical_devices, _physical_devices);
        for(uint32_t i = 0; i < num_physical_devices; i ++){
            if(_physical_devices[i] != NULL){
                if(_CheckPhysicalDeviceExtensions(_physical_devices[i]) == VK_FALSE){
                    _physical_devices[i] = NULL;
                }
            }
        }
        for(uint32_t i = 0; i < num_physical_devices; i ++){
            if(_physical_devices[i] != NULL){
                struct QueueFamilies qfamilies = _GetQueueFamilies(_physical_devices[i]);
                if(qfamilies.graphics_queue_index == -1 || qfamilies.present_queue_index == -1){
                    _physical_devices[i] = NULL;
                }
            }
        }
        for(uint32_t i = 0; i < num_physical_devices; i ++){
            if(_physical_devices[i] != NULL){
                if(_CheckPhysicalDeviceSurfaceCapabilities(_physical_devices[i]) == VK_FALSE){
                    _physical_devices[i] = NULL;
                }
            }
        }
        uint32_t index = 0;
        for(uint32_t i = 0; i < num_physical_devices; i ++){
            if(_physical_devices[i] != NULL){
                VkPhysicalDeviceProperties properties = {0};
                vkGetPhysicalDeviceProperties(_physical_devices[i], &properties);
                if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
                    index = i;
                    break;
                }
            }
        }
        _SetCurrentPhysicalDevice(_physical_devices[index]);
    }


    {   // -----    7   -----
        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo* queues_create_ci = malloc(sizeof(VkDeviceQueueCreateInfo) * _pdevice.qfamilies.num_queues);
        for(uint32_t i = 0; i < _pdevice.qfamilies.num_queues; i ++){
            queues_create_ci[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queues_create_ci[i].queueCount = 1;
            queues_create_ci[i].pQueuePriorities = &queue_priority;
            queues_create_ci[i].flags = 0;
            queues_create_ci[i].pNext = NULL;
        }
        queues_create_ci[0].queueFamilyIndex = _pdevice.qfamilies.graphics_queue_index;
        queues_create_ci[1].queueFamilyIndex = _pdevice.qfamilies.present_queue_index;

        VkPhysicalDeviceFeatures features = {0};

        VkDeviceCreateInfo device_ci = {0};
        device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_ci.pNext = NULL;
        device_ci.pQueueCreateInfos = queues_create_ci;
        device_ci.queueCreateInfoCount = _pdevice.qfamilies.num_queues;
        device_ci.pEnabledFeatures = &features;
        device_ci.enabledLayerCount = 0;
        device_ci.ppEnabledLayerNames = NULL;
        device_ci.enabledExtensionCount = NUM_NEEDED_PHYSICAL_DEVICE_EXTENSIONS;
        device_ci.ppEnabledExtensionNames = _needed_physical_device_extensions;

        VkBool32 result;
        result = vkCreateDevice(_pdevice.handle, &device_ci, NULL, &_vk_device);
        if(result != VK_SUCCESS){
            fprintf(stderr, "VK ERROR %d: failed to create logical device\n", result);
            exit(EXIT_FAILURE);
        }
    }


    {   // -----    8   -----
        vkGetDeviceQueue(_vk_device, _pdevice.qfamilies.graphics_queue_index, 0, &_graphics_queue);
        vkGetDeviceQueue(_vk_device, _pdevice.qfamilies.present_queue_index, 0, &_present_queue);
    }


    {   // -----    9   -----
        VkSurfaceFormatKHR chosen_surface_format = _pdevice.formats[0];
        for(uint32_t i = 0; i < _pdevice.num_formats; i ++){
            VkFormat format = _pdevice.formats[i].format;
            VkColorSpaceKHR color_space = _pdevice.formats[i].colorSpace;
            if(format == VK_FORMAT_B8G8R8A8_SRGB && color_space == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
                chosen_surface_format = _pdevice.formats[i];
                break;
            }
        }

        VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for(uint32_t i = 0; i < _pdevice.num_present_modes; i ++){
            VkPresentModeKHR present_mode = _pdevice.present_modes[i];
            if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR){
                chosen_present_mode = present_mode;
                break;
            }
        }

        VkExtent2D extent = {0};
        VkSurfaceCapabilitiesKHR c = _pdevice.capabilities;
        if(c.currentExtent.width != UINT32_MAX){
            extent = c.currentExtent;
        } else {
            int width, height;
            SDL_Vulkan_GetDrawableSize(_sdl_window, &width, &height);

            VkExtent2D actual_extent = {
                .width = (uint32_t)width,
                .height = (uint32_t)height,
            };

            uint32_t max_width = c.maxImageExtent.width < actual_extent.width
                ? c.maxImageExtent.width
                : actual_extent.width;
            
            uint32_t max_height = c.maxImageExtent.height < actual_extent.height
                ? c.maxImageExtent.height
                : actual_extent.height;

            uint32_t min_width = c.minImageExtent.width;
            uint32_t min_height = c.minImageExtent.height;

            extent.width = max_width < min_width ? min_width : max_width;
            extent.height = max_height < min_height ? min_height : max_height;
        }


        uint32_t num_images = c.minImageCount + 1;
        if(c.maxImageCount > 0 && num_images > c.maxImageCount){
            num_images = c.maxImageCount;
        }

        VkSwapchainCreateInfoKHR ci = {0};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.pNext = NULL;
        ci.surface = _vk_surface;
        ci.minImageCount = num_images;
        ci.imageFormat = chosen_surface_format.format;
        ci.imageColorSpace = chosen_surface_format.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        struct QueueFamilies qfamilies = _pdevice.qfamilies;
        uint32_t queue_families_indices[] = {
            qfamilies.graphics_queue_index,
            qfamilies.present_queue_index
        };

        if(qfamilies.graphics_queue_index != qfamilies.present_queue_index){
            ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices = queue_families_indices;
        } else {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.queueFamilyIndexCount = 0;
            ci.pQueueFamilyIndices = NULL;
        }

        ci.preTransform = c.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = chosen_present_mode;
        ci.clipped = VK_TRUE;
        ci.oldSwapchain = NULL;

        VkResult result;
        result = vkCreateSwapchainKHR(_vk_device, &ci, NULL, &_swap_chain.handle);
        if(result != VK_SUCCESS){
            fprintf(stderr, "VK ERROR %d: failed to create swap chain\n", result);
            exit(EXIT_FAILURE);
        }

        vkGetSwapchainImagesKHR(_vk_device, _swap_chain.handle, &_swap_chain.num_images, NULL);
        _swap_chain.images = malloc(sizeof(VkImage) * _swap_chain.num_images);
        vkGetSwapchainImagesKHR(_vk_device, _swap_chain.handle, &_swap_chain.num_images, _swap_chain.images);

        _swap_chain.image_format = chosen_surface_format.format;
        _swap_chain.extent = extent;

        _swap_chain.image_views = malloc(sizeof(VkImageView) * _swap_chain.num_images);
        for(uint32_t i = 0; i < _swap_chain.num_images; i ++){
            VkImageViewCreateInfo image_view_ci = {0};
            image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_ci.pNext = NULL;
            image_view_ci.image = _swap_chain.images[i];
            image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_ci.format = _swap_chain.image_format;
            image_view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_ci.subresourceRange.baseMipLevel = 0;
            image_view_ci.subresourceRange.levelCount = 1;
            image_view_ci.subresourceRange.baseArrayLayer = 0;
            image_view_ci.subresourceRange.layerCount = 1;
            result = vkCreateImageView(_vk_device, &image_view_ci, NULL, &_swap_chain.image_views[i]);
            if(result != VK_SUCCESS){
                fprintf(stderr, "VK ERROR %d: failed to create image view #%d\n", result, i);
                exit(EXIT_FAILURE);
            }
        }

        #ifdef DEBUG
            printf("SWAP CHAIN CREATION SUCCESSFUL\n{\n");
            printf("\tSurface format: %d\n", chosen_surface_format.format);
            printf("\tSurface color space: %d\n", chosen_surface_format.colorSpace);
            printf("\tPresent mode: %d\n", chosen_present_mode);
            printf("\tExtent width: %d\n", extent.width);
            printf("\tExtent height: %d\n", extent.height);
            printf("\tMin images: %d\n", num_images);
            printf("\tNum images: %d\n", _swap_chain.num_images);
            if(ci.imageSharingMode == VK_SHARING_MODE_CONCURRENT){
                printf("\tSharing mode: CONCURRENT\n");
            } else {
                printf("\tSharing mode: EXCLUSIVE\n");
            }
            printf("}\n\n");
        #endif
    }


    {   // -----    10  -----

    }

}

void FREE_VREND(){
    for(uint32_t i = 0; i < _swap_chain.num_images; i ++){
        vkDestroyImageView(_vk_device, _swap_chain.image_views[i], NULL);
    }
    vkDestroySwapchainKHR(_vk_device, _swap_chain.handle, NULL);
    vkDestroyDevice(_vk_device, NULL);
    vkDestroySurfaceKHR(_vk_instance, _vk_surface, NULL);
    #ifdef DEBUG
        FreeDebugUtils(&_vk_instance);
    #endif
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
        .present_queue_index = -1,
        .num_queues = 2
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

    // Check if there are surface formats
    uint32_t num_formats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, _vk_surface, &num_formats, NULL);
    if(num_formats == 0){
        return VK_FALSE;
    }

    // Check if there are surface present modes
    uint32_t num_present_modes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, _vk_surface, &num_present_modes, NULL);
    if(num_present_modes == 0){
        return VK_FALSE;
    }

    return VK_TRUE;
}

void _SetCurrentPhysicalDevice(VkPhysicalDevice physical_device){

    // Properties, memory properties, and features
    _pdevice.handle = physical_device;
    vkGetPhysicalDeviceProperties(physical_device, &_pdevice.properties);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &_pdevice.memory_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &_pdevice.features);

    // Capabilities, formats, and present modes
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, _vk_surface, &_pdevice.capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, _vk_surface, &_pdevice.num_formats, NULL);
    free(_pdevice.formats);
    _pdevice.formats = malloc(sizeof(VkSurfaceFormatKHR) * _pdevice.num_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, _vk_surface,
        &_pdevice.num_formats,
        _pdevice.formats
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, _vk_surface, &_pdevice.num_present_modes, NULL);
    free(_pdevice.present_modes);
    _pdevice.present_modes = malloc(sizeof(VkPresentModeKHR) * _pdevice.num_present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, _vk_surface,
        &_pdevice.num_present_modes,
        _pdevice.present_modes
    );

    _pdevice.qfamilies = _GetQueueFamilies(physical_device);
}