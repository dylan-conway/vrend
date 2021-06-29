#include "vrend.h"

struct PhysicalDeviceInfo {
    VkPhysicalDevice                        handle;

    uint32_t                                graphics_queue_index;
    uint32_t                                present_queue_index;
    uint32_t                                num_queues;

    VkPhysicalDeviceProperties              properties;
    VkPhysicalDeviceMemoryProperties        mem_properties;
    VkPhysicalDeviceFeatures                features;
    VkSurfaceCapabilitiesKHR                capabilities;
    VkPhysicalDeviceLimits                  limits;

    uint32_t                                num_formats;            
    VkSurfaceFormatKHR*                     formats;
    uint32_t                                num_present_modes;
    VkPresentModeKHR*                       present_modes;
};

struct SwapChainInfo {
    VkSwapchainKHR                          handle;
    VkFormat                                format;
    uint32_t                                num_images;
    VkImage*                                images;
    VkImageView*                            image_views;
    VkFramebuffer*                          framebuffers;
};

#define NUM_REQUIRED_PHYSICAL_DEVICE_EXTENSIONS 1
static const char* _required_physical_device_extensions[NUM_REQUIRED_PHYSICAL_DEVICE_EXTENSIONS] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static uint32_t                         _frame_counter = 0;
static VkExtent2D                       _window_extent = {0};
static SDL_Window*                      _window = NULL;
static VkInstance                       _instance = NULL;
static VkSurfaceKHR                     _surface = NULL;
static struct PhysicalDeviceInfo        _physical_device = {0};
static VkDevice                         _device = NULL;
static VkQueue                          _graphics_queue = NULL;
static VkQueue                          _present_queue = NULL;
static VkCommandPool                    _command_pool = NULL;
static VkSemaphore                      _present_semaphore = NULL;
static VkSemaphore                      _render_semaphore = NULL;
static VkFence                          _render_fence = NULL;

// Needs to be remade on swap chain creation
static struct SwapChainInfo             _swap_chain = {0};
static VkCommandBuffer                  _command_buffer = NULL;
static VkRenderPass                     _render_pass = NULL;
static VkPipelineLayout                 _pipeline_layout = NULL;
static VkPipeline                       _pipeline = NULL;

VkBool32 _CheckInstanceExtensions();
void _SetPhysicalDevice(VkPhysicalDevice device);
void _CreateSwapChain();
void _CreateCommandBuffers();
void _CreateRenderPass();
void _LoadShaderModule(char* path, VkShaderModule* module);
void _CreateGraphicsPipeline();
void _CreateFramebuffers();
void CHECK(VkResult result, char* fname, VkBool32 print);
void _PrintVulkanFunctionName(char* fname);

void INIT_VREND(char* title, uint32_t w, uint32_t h){

    {   // Initialize SDL2 window with Vulkan flag
        SDL_Init(SDL_INIT_EVERYTHING);
        SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
        _window = SDL_CreateWindow(
            title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            w, h, flags
        );
        if(_window == NULL){
            fprintf(stderr, "ERROR: failed to create SDL2 window\n");
            exit(EXIT_FAILURE);
        }
    }



    {   // Check layers and extensions for instance
        #ifdef DEBUG
            if(!CheckInstanceLayers()){
                fprintf(stderr, "ERROR: failed to find required Vulkan instance layers\n");
                exit(EXIT_FAILURE);
            }
        #endif
        if(!_CheckInstanceExtensions()){
            fprintf(stderr, "ERROR: failed to find required Vulkan instance extensions\n");
            exit(EXIT_FAILURE);
        }
    }



    {   // Vulkan instance
        VkApplicationInfo app_info = {0};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = NULL;
        app_info.pApplicationName = title;
        app_info.pEngineName = "No engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        uint32_t num_instance_extensions = 0;
        SDL_Vulkan_GetInstanceExtensions(_window, &num_instance_extensions, NULL);
        const char** instance_extensions = malloc(sizeof(char*) * num_instance_extensions);
        SDL_Vulkan_GetInstanceExtensions(_window, &num_instance_extensions, instance_extensions);
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

        VK_CHECK(vkCreateInstance, &instance_ci, NULL, &_instance);

        free(instance_extensions);
    }



    {   // Debug utils
        #ifdef DEBUG
            InitDebugUtils(&_instance);
        #endif
    }



    {   // SDL2 surface for Vulkan
        SDL_bool result;
        result = SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
        if(result != SDL_TRUE){
            fprintf(stderr, "SDL2 ERROR: failed to create SDL2 surface for Vulkan\n");
            exit(EXIT_FAILURE);
        }
    }



    {   // Choose GPU
        // (Chooses first available discrete gpu)
        // NEEDS WORK
        uint32_t num_physical_devices = 0;
        vkEnumeratePhysicalDevices(_instance, &num_physical_devices, NULL);
        if(num_physical_devices == 0){
            fprintf(stderr, "ERROR: failed to find a device with Vulkan support\n");
            exit(EXIT_FAILURE);
        }
        VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * num_physical_devices);
        vkEnumeratePhysicalDevices(_instance, &num_physical_devices, physical_devices);
        // for(uint32_t i = 0; i < num_physical_devices; i ++){
        //     if(physical_devices[i] != NULL){
        //         if(_CheckPhysicalDeviceExtensions(physical_devices[i]) == VK_FALSE){
        //             physical_devices[i] = NULL;
        //         }
        //     }
        // }
        // for(uint32_t i = 0; i < num_physical_devices; i ++){
        //     if(physical_devices[i] != NULL){
        //         struct QueueFamilies qfamilies = _GetQueueFamilies(_physical_devices[i]);
        //         if(qfamilies.graphics_queue_index == -1 || qfamilies.present_queue_index == -1){
        //             _physical_devices[i] = NULL;
        //         }
        //     }
        // }
        // for(uint32_t i = 0; i < num_physical_devices; i ++){
        //     if(_physical_devices[i] != NULL){
        //         if(_CheckPhysicalDeviceSurfaceCapabilities(_physical_devices[i]) == VK_FALSE){
        //             _physical_devices[i] = NULL;
        //         }
        //     }
        // }
        for(uint32_t i = 0; i < num_physical_devices; i ++){
            VkPhysicalDeviceProperties properties = {0};
            vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
            if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
                _SetPhysicalDevice(physical_devices[i]);
                break;
            }
        }

        free(physical_devices);

        #ifdef DEBUG
            // printf("PHYSICAL DEVICE SELECTION SUCCESSFUL\n{\n");
            // printf("\tDevice: %s\n", _physical_device.properties.deviceName);
            // printf("\tHEAP SIZES:\n\t{\n");
            // for(uint32_t i = 0; i < _physical_device.mem_properties.memoryHeapCount; i ++){
            //     printf("\t\t#%d size: %I64i bytes\n", i, _physical_device.mem_properties.memoryHeaps[i].size);
            // }
            // printf("\t}\n");
            // printf("\tHEAP TYPES:\n\t{\n");
            // for(uint32_t i = 0; i < _physical_device.mem_properties.memoryTypeCount; i ++){
            //     if(_physical_device.mem_properties.memoryTypes[i].propertyFlags != 0){
            //         printf("\t\t#%d ", _physical_device.mem_properties.memoryTypes[i].heapIndex);
            //     }
            //     switch(_physical_device.mem_properties.memoryTypes[i].propertyFlags){
            //         case 0:
            //             // printf("%s\n", "zero");
            //             break;
            //         case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
            //             printf("%s\n", "HOST_VISIBLE | HOST_COHERENT");
            //             break;
            //         case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT:
            //             printf("%s\n", "HOST_VISIBLE | HOST_CACHED");
            //             break;
            //         case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
            //             printf("%s\n", "HOST_VISIBLE | HOST_CACHED | HOST_COHERENT");
            //             break;
            //         case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:
            //             printf("%s\n", "DEVICE_LOCAL");
            //             break;
            //         case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
            //             printf("%s\n", "DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT");
            //             break;
            //         case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT:
            //             printf("%s\n", "DEVICE_LOCAL | HOST_VISIBLE | HOST_CACHED");
            //             break;
            //         case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
            //             printf("%s\n", "DEVICE_LOCAL | HOST_VISIBLE | HOST_CACHED | HOST_COHERENT");
            //             break;
            //         case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT:
            //             printf("%s\n", "DEVICE_LOCAL | LAZILY_ALLOCATED");
            //             break;
            //         case VK_MEMORY_PROPERTY_PROTECTED_BIT:
            //             printf("%s\n", "PROTECTED");
            //             break;
            //         case VK_MEMORY_PROPERTY_PROTECTED_BIT | VK_MEMORY_HEAP_DEVICE_LOCAL_BIT:
            //             printf("%s\n", "PROTECTED | DEVICE_LOCAL");
            //             break;
            //         default:
            //             printf("UNKNOWN\n");
            //             break;
            //     }
            // }
            // printf("\t}\n");
            // printf("}\n\n");
        #endif
    }



    {   // Create logical device and get device queues
        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo* queues_create_ci = malloc(sizeof(VkDeviceQueueCreateInfo) * _physical_device.num_queues);
        for(uint32_t i = 0; i < _physical_device.num_queues; i ++){
            queues_create_ci[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queues_create_ci[i].queueCount = 1;
            queues_create_ci[i].pQueuePriorities = &queue_priority;
            queues_create_ci[i].flags = 0;
            queues_create_ci[i].pNext = NULL;
        }
        queues_create_ci[0].queueFamilyIndex = _physical_device.graphics_queue_index;
        queues_create_ci[1].queueFamilyIndex = _physical_device.present_queue_index;

        VkPhysicalDeviceFeatures features = {0};

        VkDeviceCreateInfo device_ci = {0};
        device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_ci.pNext = NULL;
        device_ci.pQueueCreateInfos = queues_create_ci;
        device_ci.queueCreateInfoCount = _physical_device.num_queues;
        device_ci.pEnabledFeatures = &features;
        device_ci.enabledLayerCount = 0;
        device_ci.ppEnabledLayerNames = NULL;
        device_ci.enabledExtensionCount = NUM_REQUIRED_PHYSICAL_DEVICE_EXTENSIONS;
        device_ci.ppEnabledExtensionNames = _required_physical_device_extensions;

        VK_CHECK(vkCreateDevice, _physical_device.handle, &device_ci, NULL, &_device);

        vkGetDeviceQueue(_device, _physical_device.graphics_queue_index, 0, &_graphics_queue);
        vkGetDeviceQueue(_device, _physical_device.present_queue_index, 0, &_present_queue);
    }

    {   // Create command pool

        VkCommandPoolCreateInfo command_pool_ci = GetCommandPoolCI(
            _physical_device.graphics_queue_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
        );
        VK_CHECK(vkCreateCommandPool, _device, &command_pool_ci, NULL, &_command_pool);

    }

    {   // Swap chain creation
        _CreateSwapChain();
    }

    {   // Initialize sync structures

        VkFenceCreateInfo fence_ci = GetFenceCI(VK_FENCE_CREATE_SIGNALED_BIT);
        VK_CHECK(vkCreateFence, _device, &fence_ci, NULL, &_render_fence);

        VkSemaphoreCreateInfo semaphore_ci = GetSemaphoreCI(0);
        VK_CHECK(vkCreateSemaphore, _device, &semaphore_ci, NULL, &_present_semaphore);
        VK_CHECK(vkCreateSemaphore, _device, &semaphore_ci, NULL, &_render_semaphore);
    }

}

void FREE_VREND(){

    vkDeviceWaitIdle(_device);

    vkDestroyPipeline(_device, _pipeline, NULL);
    vkDestroyPipelineLayout(_device, _pipeline_layout, NULL);

    vkDestroyFence(_device, _render_fence, NULL);
    vkDestroySemaphore(_device, _render_semaphore, NULL);
    vkDestroySemaphore(_device, _present_semaphore, NULL);

    vkDestroyRenderPass(_device, _render_pass, NULL);
    vkFreeCommandBuffers(_device, _command_pool, 1, &_command_buffer);
    vkDestroyCommandPool(_device, _command_pool, NULL);
    for(uint32_t i = 0; i < _swap_chain.num_images; i ++){
        vkDestroyFramebuffer(_device, _swap_chain.framebuffers[i], NULL);
        vkDestroyImageView(_device, _swap_chain.image_views[i], NULL);
    }
    free(_swap_chain.images);
    free(_swap_chain.image_views);
    vkDestroySwapchainKHR(_device, _swap_chain.handle, NULL);
    vkDestroyDevice(_device, NULL);
    vkDestroySurfaceKHR(_instance, _surface, NULL);
    #ifdef DEBUG
        FreeDebugUtils(&_instance);
    #endif
    vkDestroyInstance(_instance, NULL);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void _CreateSwapChain(){

    vkDeviceWaitIdle(_device);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device.handle, _surface, &_physical_device.capabilities);
    _window_extent = _physical_device.capabilities.currentExtent;

    while(_window_extent.width == 0 || _window_extent.height == 0){
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device.handle, _surface, &_physical_device.capabilities);
        _window_extent = _physical_device.capabilities.currentExtent;
        SDL_Event event;

        // Poll events in order to leave minimized mode
        SDL_WaitEvent(&event);
    }

    VkSurfaceCapabilitiesKHR c = _physical_device.capabilities;

    uint32_t num_images = c.minImageCount + 1;
    if(c.minImageCount > 0 && num_images > c.maxImageCount){
        num_images = c.maxImageCount;
    }

    // Free structures if made before
    if(_swap_chain.handle){
        for(uint32_t i = 0; i < num_images; i ++){
            vkDestroyFramebuffer(_device, _swap_chain.framebuffers[i], NULL);
        }
        free(_swap_chain.framebuffers);
        vkDestroyPipeline(_device, _pipeline, NULL);
        vkDestroyPipelineLayout(_device, _pipeline_layout, NULL);
        vkDestroyRenderPass(_device, _render_pass, NULL);
        vkFreeCommandBuffers(_device, _command_pool, 1, &_command_buffer);
        for(uint32_t i = 0; i < num_images; i ++){
            vkDestroyImageView(_device, _swap_chain.image_views[i], NULL);
        }
        free(_swap_chain.image_views);
        vkDestroySwapchainKHR(_device, _swap_chain.handle, NULL);
    }

    VkSurfaceFormatKHR chosen_format = _physical_device.formats[0];
    for(uint32_t i = 0; i < _physical_device.num_formats; i ++){
        VkFormat format = _physical_device.formats[i].format;
        VkColorSpaceKHR color_space = _physical_device.formats[i].colorSpace;
        if(format == VK_FORMAT_B8G8R8A8_UNORM && color_space == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            chosen_format = _physical_device.formats[i];
            break;
        }
    }

    VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(uint32_t i = 0; i < _physical_device.num_present_modes; i ++){
        VkPresentModeKHR present_mode = _physical_device.present_modes[i];
        if(present_mode == VK_PRESENT_MODE_MAILBOX_KHR){
            chosen_present_mode = present_mode;
            break;
        }
    }

    VkSwapchainCreateInfoKHR ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.pNext = NULL;
    ci.surface = _surface;
    ci.minImageCount = num_images;
    ci.imageFormat = chosen_format.format;
    ci.imageColorSpace = chosen_format.colorSpace;
    ci.imageExtent = _window_extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_families_indices[] = {
        _physical_device.graphics_queue_index,
        _physical_device.present_queue_index
    };

    if(_physical_device.graphics_queue_index != _physical_device.present_queue_index){
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = _physical_device.num_queues;
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

    VK_CHECK(vkCreateSwapchainKHR, _device, &ci, NULL, &_swap_chain.handle);

    vkGetSwapchainImagesKHR(_device, _swap_chain.handle, &_swap_chain.num_images, NULL);
    _swap_chain.images = malloc(sizeof(VkImage) * _swap_chain.num_images);
    vkGetSwapchainImagesKHR(_device, _swap_chain.handle, &_swap_chain.num_images, _swap_chain.images);

    _swap_chain.format = chosen_format.format;

    _swap_chain.image_views = malloc(sizeof(VkImageView) * _swap_chain.num_images);
    for(uint32_t i = 0; i < _swap_chain.num_images; i ++){
        VkImageViewCreateInfo image_view_ci = {0};
        image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_ci.pNext = NULL;
        image_view_ci.image = _swap_chain.images[i];
        image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_ci.format = _swap_chain.format;
        image_view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_ci.subresourceRange.baseMipLevel = 0;
        image_view_ci.subresourceRange.levelCount = 1;
        image_view_ci.subresourceRange.baseArrayLayer = 0;
        image_view_ci.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView, _device, &image_view_ci, NULL, &_swap_chain.image_views[i]);
    }

    #ifdef DEBUG
        // printf("SWAP CHAIN CREATION SUCCESSFUL\n{\n");
        // printf("\tSurface format: %s\n", STR_VK_FORMAT(chosen_format.format));
        // printf("\tSurface color space: %s\n", STR_VK_COLOR_SPACE_KHR(chosen_format.colorSpace));
        // printf("\tPresent mode: %s\n", STR_VK_PRESENT_MODE_KHR(chosen_present_mode));
        // printf("\tSharing mode: %s\n", STR_VK_SHARING_MODE(ci.imageSharingMode));
        // printf("\tExtent width: %d\n", extent.width);
        // printf("\tExtent height: %d\n", extent.height);
        // printf("\tMin images: %d\n", num_images);
        // printf("\tNum images: %d\n", _swap_chain.num_images);
        // printf("}\n\n");
    #endif

    _CreateCommandBuffers();
    _CreateRenderPass();
    _CreateGraphicsPipeline();
    _CreateFramebuffers();
}

void _CreateCommandBuffers(){
    VkCommandBufferAllocateInfo command_buffer_ai = GetCommandBufferAI(
        _command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY
    );
    VK_CHECK(vkAllocateCommandBuffers, _device, &command_buffer_ai, &_command_buffer);
}

void _CreateRenderPass(){
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = _swap_chain.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency subpass_dependency = {0};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_ci = GetRenderPassCI(
        1, &color_attachment,
        1, &subpass,
        1, &subpass_dependency
    );
    VK_CHECK(vkCreateRenderPass, _device, &render_pass_ci, NULL, &_render_pass);
}

void _CreateGraphicsPipeline(){
    
    VkPipelineLayoutCreateInfo pipeline_layout = GetPipelineLayoutCI();
    VK_CHECK(vkCreatePipelineLayout, _device, &pipeline_layout, NULL, &_pipeline_layout);

    VkShaderModule vert_shader_module = NULL;
    _LoadShaderModule("src/vert.spv", &vert_shader_module);

    VkShaderModule frag_shader_module = NULL;
    _LoadShaderModule("src/frag.spv", &frag_shader_module);

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        GetShaderStageCI(VK_SHADER_STAGE_VERTEX_BIT, vert_shader_module),
        GetShaderStageCI(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_module)
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)_window_extent.width,
        .height = (float)_window_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {
            .x = 0,
            .y = 0
        },
        .extent = _window_extent
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_ci = GetVertexInputCI();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = GetInputAssemblyCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    VkPipelineRasterizationStateCreateInfo rasterization_ci = GetRasterizationCI(VK_POLYGON_MODE_FILL);
    VkPipelineMultisampleStateCreateInfo multisample_ci = GetMultisampleCI();

    VkPipelineColorBlendAttachmentState color_blend_attachment = GetColorBlendAttachmentState();

    VkPipelineColorBlendStateCreateInfo color_blend_ci = {0};
    color_blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_ci.pNext = NULL;
    color_blend_ci.logicOpEnable = VK_FALSE;
    color_blend_ci.logicOp = VK_LOGIC_OP_COPY;
    color_blend_ci.attachmentCount = 1;
    color_blend_ci.pAttachments = &color_blend_attachment;

    VkPipelineViewportStateCreateInfo viewport_ci = {0};
    viewport_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_ci.pNext = NULL;
    viewport_ci.viewportCount = 1;
    viewport_ci.pViewports = &viewport;
    viewport_ci.scissorCount = 1;
    viewport_ci.pScissors = &scissor;

    VkGraphicsPipelineCreateInfo pipeline_ci = {0};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = NULL;
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = shader_stages;
    pipeline_ci.pVertexInputState = &vertex_input_ci;
    pipeline_ci.pViewportState = &viewport_ci;
    pipeline_ci.pRasterizationState = &rasterization_ci;
    pipeline_ci.pMultisampleState = &multisample_ci;
    pipeline_ci.pColorBlendState = &color_blend_ci;
    pipeline_ci.pInputAssemblyState = &input_assembly_ci;
    pipeline_ci.layout = _pipeline_layout;
    pipeline_ci.renderPass = _render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = NULL;
    
    VK_CHECK(vkCreateGraphicsPipelines, _device, NULL, 1, &pipeline_ci, NULL, &_pipeline);

    vkDestroyShaderModule(_device, vert_shader_module, NULL);
    vkDestroyShaderModule(_device, frag_shader_module, NULL);
}

void _CreateFramebuffers(){
    VkFramebufferCreateInfo framebuffer_ci = {0};
    framebuffer_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.pNext = NULL;
    framebuffer_ci.renderPass = _render_pass;
    framebuffer_ci.attachmentCount = 1;
    framebuffer_ci.width = _window_extent.width;
    framebuffer_ci.height = _window_extent.height;
    framebuffer_ci.layers = 1;

    _swap_chain.framebuffers = malloc(sizeof(VkFramebuffer) * _swap_chain.num_images);
    for(uint32_t i = 0; i < _swap_chain.num_images; i ++){
        framebuffer_ci.pAttachments = &_swap_chain.image_views[i];
        VK_CHECK(vkCreateFramebuffer, _device, &framebuffer_ci, NULL, &_swap_chain.framebuffers[i]);
    }
}

void DRAW_VREND(){

    VK_CHECK_S(vkWaitForFences, _device, 1, &_render_fence, VK_TRUE, UINT64_MAX);
    VK_CHECK_S(vkResetFences, _device, 1, &_render_fence);

    uint32_t image_index;
    VkResult result;
    result = vkAcquireNextImageKHR(_device, _swap_chain.handle, UINT64_MAX, _present_semaphore, NULL, &image_index);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){
        _CreateSwapChain();
        return;
    }

    VK_CHECK_S(vkResetCommandBuffer, _command_buffer, 0);

    VkCommandBufferBeginInfo command_buffer_bi = GetCommandBufferBI(
        NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    );
    VK_CHECK_S(vkBeginCommandBuffer, _command_buffer, &command_buffer_bi);

    VkClearValue clear_value = {0};
    float flash = fabs(sin(_frame_counter / 120.0f));
    clear_value.color.float32[0] = 0.0f;
    clear_value.color.float32[1] = 0.0f;
    clear_value.color.float32[2] = flash;
    clear_value.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo render_pass_bi = GetRenderPassBI(
        _render_pass, (VkOffset2D){0, 0}, _window_extent, _swap_chain.framebuffers[image_index],
        1, &clear_value
    );

    vkCmdBeginRenderPass(_command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vkCmdDraw(_command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(_command_buffer);

    VK_CHECK_S(vkEndCommandBuffer, _command_buffer);

    VkSubmitInfo submit = {0};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = NULL;
    
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &wait_stage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &_present_semaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &_render_semaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &_command_buffer;

    VK_CHECK_S(vkQueueSubmit, _graphics_queue, 1, &submit, _render_fence);

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swap_chain.handle;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_render_semaphore;
    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(_graphics_queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR){
        _CreateSwapChain();
        return;
    }

    _frame_counter += 1;
}

VkBool32 _CheckInstanceExtensions(){

    // Get the needed extensions for the platform (SDL2)
    uint32_t num_needed_extensions = 0;
    SDL_Vulkan_GetInstanceExtensions(_window, &num_needed_extensions, NULL);
    const char** needed_extensions = malloc(sizeof(char*) * num_needed_extensions);
    SDL_Vulkan_GetInstanceExtensions(_window, &num_needed_extensions, needed_extensions);

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

void _SetPhysicalDevice(VkPhysicalDevice device){
    _physical_device.handle = device;
    _physical_device.num_queues = 2;
    
    uint32_t num_queues = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queues, NULL);
    VkQueueFamilyProperties* queue_properties = malloc(sizeof(VkQueueFamilyProperties) * num_queues);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queues, queue_properties);

    for(uint32_t i = 0; i < num_queues; i ++){
        VkQueueFlags flags = queue_properties[i].queueFlags;
        if((flags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT){
            _physical_device.graphics_queue_index = i;
        }

        VkBool32 has_present_family = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i , _surface, &has_present_family);
        if(has_present_family){
            _physical_device.present_queue_index = i;
        }
    }

    free(queue_properties);

    vkGetPhysicalDeviceProperties(device, &_physical_device.properties);
    vkGetPhysicalDeviceMemoryProperties(device, &_physical_device.mem_properties);
    vkGetPhysicalDeviceFeatures(device, &_physical_device.features);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &_physical_device.capabilities);

    _window_extent = _physical_device.capabilities.currentExtent;

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, _surface, &_physical_device.num_formats, NULL
    );
    _physical_device.formats = malloc(sizeof(VkSurfaceFormatKHR) * _physical_device.num_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, _surface, &_physical_device.num_formats, _physical_device.formats
    );

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, _surface, &_physical_device.num_present_modes, NULL
    );
    free(_physical_device.present_modes);
    _physical_device.present_modes = malloc(sizeof(VkPresentModeKHR) * _physical_device.num_present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, _surface, &_physical_device.num_present_modes, _physical_device.present_modes
    );
}

void _LoadShaderModule(char* path, VkShaderModule* module){
    
    FILE* file = fopen(path, "rb");
    if(file == NULL){
        fprintf(stderr, "ERROR: failed to open shader file %s\n", path);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* shader_code = malloc(sizeof(char) * size);
    fread(shader_code, size, sizeof(char), file);

    fclose(file);

    VkShaderModuleCreateInfo module_ci = {0};
    module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_ci.pNext = NULL;
    module_ci.flags = 0;
    module_ci.codeSize = size;
    module_ci.pCode = (uint32_t*)shader_code;

    VkShaderModule temp_module = NULL;
    VK_CHECK(vkCreateShaderModule, _device, &module_ci, NULL, &temp_module);

    *module = temp_module;
}

void CHECK(VkResult result, char* fname, VkBool32 print){

    #ifdef DEBUG
        char* m = STR_VK_RESULT(result);
        if(result < 0){
            // Always print on exit
            printf("\e[0;31m%s\e[0m @", m);
            _PrintVulkanFunctionName(fname);
            exit(EXIT_FAILURE);
        }
        if(result == VK_SUCCESS){
            if(print){
                printf("\e[0;32m%s\e[0m @", m);
                _PrintVulkanFunctionName(fname);
            }
        } else {
            printf("\e[0;33m%s\e[0m @", m);
            _PrintVulkanFunctionName(fname);
        }
    #endif

    if(result < 0){
        exit(EXIT_FAILURE);
    }
}

void _PrintVulkanFunctionName(char* fname){
    for(uint32_t i = 2; i < strlen(fname); i ++){
        if(isupper(fname[i]) && islower(fname[i - 1])) printf(" ");
        printf("%c", fname[i]);
    }
    printf("\n");
}
