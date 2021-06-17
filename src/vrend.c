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

struct PipelineDetails {
    VkPipeline handle;
    VkPipelineLayout layout;
};

VkBool32 _CheckInstanceExtensions();
VkBool32 _CheckPhysicalDeviceExtensions(VkPhysicalDevice physical_device);
struct QueueFamilies _GetQueueFamilies(VkPhysicalDevice physical_device);
VkBool32 _CheckPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device);
void _SetCurrentPhysicalDevice(VkPhysicalDevice physical_device);
char* _ReadSPVFile(char* filename, size_t* out_size);
VkShaderModule _CreateShaderModule(char* shader_code, size_t size);

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
static VkRenderPass _render_pass = NULL;

static struct PipelineDetails _pipeline = {0};

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

        VkAttachmentDescription color_attachment = {0};
        color_attachment.format = _swap_chain.image_format;
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

        VkRenderPassCreateInfo render_pass_ci = {0};
        render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_ci.pNext = NULL;
        render_pass_ci.attachmentCount = 1;
        render_pass_ci.pAttachments = &color_attachment;
        render_pass_ci.subpassCount = 1;
        render_pass_ci.pSubpasses = &subpass;
        render_pass_ci.dependencyCount = 1;
        render_pass_ci.pDependencies = &subpass_dependency;

        VkResult result;
        result = vkCreateRenderPass(_vk_device, &render_pass_ci, NULL, &_render_pass);
        if(result != VK_SUCCESS){
            fprintf(stderr, "VK ERROR %d: failed to create render pass\n", result);
        }

        size_t vert_shader_size, frag_shader_size;
        char* vert_shader_code = _ReadSPVFile("src/vert.spv", &vert_shader_size);
        char* frag_shader_code = _ReadSPVFile("src/frag.spv", &frag_shader_size);

        VkShaderModule vert_shader_module = _CreateShaderModule(vert_shader_code, vert_shader_size);
        VkShaderModule frag_shader_module = _CreateShaderModule(frag_shader_code, frag_shader_size);

        VkPipelineShaderStageCreateInfo vert_state_ci = {0};
        vert_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_state_ci.pNext = NULL;
        vert_state_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_state_ci.module = vert_shader_module;
        vert_state_ci.pName = "main";

        VkPipelineShaderStageCreateInfo frag_state_ci = {0};
        frag_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_state_ci.pNext = NULL;
        frag_state_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_state_ci.module = frag_shader_module;
        frag_state_ci.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages_ci[] = {
            vert_state_ci,
            frag_state_ci
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_ci = {0};
        vertex_input_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_ci.pNext = NULL;
        vertex_input_ci.vertexBindingDescriptionCount = 0;
        vertex_input_ci.pVertexBindingDescriptions = NULL;
        vertex_input_ci.vertexAttributeDescriptionCount = 0;
        vertex_input_ci.pVertexAttributeDescriptions = NULL;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {0};
        input_assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_ci.pNext = NULL;
        input_assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_ci.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {0};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)_swap_chain.extent.width;
        viewport.height = (float)_swap_chain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0;

        VkRect2D scissor = {0};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = _swap_chain.extent;

        VkPipelineViewportStateCreateInfo viewport_ci = {0};
        viewport_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_ci.viewportCount = 1;
        viewport_ci.pViewports = &viewport;
        viewport_ci.scissorCount = 1;
        viewport_ci.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer_ci = {0};
        rasterizer_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer_ci.pNext = NULL;
        rasterizer_ci.depthClampEnable = VK_FALSE;
        rasterizer_ci.rasterizerDiscardEnable = VK_FALSE;
        rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer_ci.lineWidth = 1.0f;
        rasterizer_ci.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer_ci.depthBiasEnable = VK_FALSE;
        rasterizer_ci.depthBiasConstantFactor = 0.0f;
        rasterizer_ci.depthBiasClamp = 0.0f;
        rasterizer_ci.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling_ci = {0};
        multisampling_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_ci.pNext = NULL;
        multisampling_ci.sampleShadingEnable = VK_FALSE;
        multisampling_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling_ci.minSampleShading = 1.0f;
        multisampling_ci.pSampleMask = NULL;
        multisampling_ci.alphaToCoverageEnable = VK_FALSE;
        multisampling_ci.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState blend_state = {0};
        blend_state.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        blend_state.blendEnable = VK_TRUE;
        blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_state.colorBlendOp = VK_BLEND_OP_ADD;
        blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_state.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blend_state_ci = {0};
        color_blend_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state_ci.pNext = NULL;
        color_blend_state_ci.logicOpEnable = VK_FALSE;
        color_blend_state_ci.logicOp = VK_LOGIC_OP_COPY;
        color_blend_state_ci.attachmentCount = 1;
        color_blend_state_ci.pAttachments = &blend_state;
        color_blend_state_ci.blendConstants[0] = 0.0f;
        color_blend_state_ci.blendConstants[1] = 0.0f;
        color_blend_state_ci.blendConstants[2] = 0.0f;
        color_blend_state_ci.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo layout_ci = {0};
        layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_ci.pNext = NULL;
        layout_ci.setLayoutCount = 0;
        layout_ci.pSetLayouts = NULL;
        layout_ci.pushConstantRangeCount = 0;
        layout_ci.pPushConstantRanges = NULL;

        result = vkCreatePipelineLayout(_vk_device, &layout_ci, NULL, &_pipeline.layout);
        if(result != VK_SUCCESS){
            fprintf(stderr, "VK ERROR %d: failed to create pipeline layout\n", result);
            exit(EXIT_FAILURE);
        }

        VkGraphicsPipelineCreateInfo pipeline_ci = {0};
        pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_ci.pNext = NULL;
        pipeline_ci.stageCount = 2;
        pipeline_ci.pStages = shader_stages_ci;
        pipeline_ci.pVertexInputState = &vertex_input_ci;
        pipeline_ci.pInputAssemblyState = &input_assembly_ci;
        pipeline_ci.pViewportState = &viewport_ci;
        pipeline_ci.pRasterizationState = &rasterizer_ci;
        pipeline_ci.pMultisampleState = &multisampling_ci;
        pipeline_ci.pDepthStencilState = NULL;
        pipeline_ci.pColorBlendState = &color_blend_state_ci;
        pipeline_ci.pDynamicState = NULL;
        pipeline_ci.layout = _pipeline.layout;
        pipeline_ci.renderPass = _render_pass;
        pipeline_ci.subpass = 0;
        pipeline_ci.basePipelineHandle = NULL;
        pipeline_ci.basePipelineIndex = -1;

        result = vkCreateGraphicsPipelines(_vk_device, NULL, 1, &pipeline_ci, NULL, &_pipeline.handle);
        if(result != VK_SUCCESS){
            fprintf(stderr, "VK ERROR %d: failed to create graphics pipeline\n", result);
            exit(EXIT_FAILURE);
        }

        vkDestroyShaderModule(_vk_device, vert_shader_module, NULL);
        vkDestroyShaderModule(_vk_device, frag_shader_module, NULL);
    }
}

void FREE_VREND(){
    vkDestroyPipeline(_vk_device, _pipeline.handle, NULL);
    vkDestroyPipelineLayout(_vk_device, _pipeline.layout, NULL);
    vkDestroyRenderPass(_vk_device, _render_pass, NULL);
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

char* _ReadSPVFile(char* filename, size_t* out_size){
    FILE* file = fopen(filename, "rb");
    if(file == NULL){
        fprintf(stderr, "ERROR: failed to open file %s\n", filename);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* buffer = malloc(sizeof(char) * size);
    fread(buffer, size, sizeof(char), file);

    fclose(file);
    *out_size = (size_t)size;
    return buffer;
}

VkShaderModule _CreateShaderModule(char* shader_code, size_t size){

    VkShaderModuleCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = size;
    ci.pCode = (uint32_t*)shader_code;

    VkShaderModule shader_module = NULL;
    VkResult result;
    result = vkCreateShaderModule(_vk_device, &ci, NULL, &shader_module);
    if(result != VK_SUCCESS){
        fprintf(stderr, "VK ERROR %d: failed to create shader module\n", result);
        exit(EXIT_FAILURE);
    }

    return shader_module;
}
