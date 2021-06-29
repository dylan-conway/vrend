
#include "vk_struct_init.h"

VkCommandPoolCreateInfo GetCommandPoolCI(uint32_t queue_family_index, VkCommandPoolCreateFlags flags){
    VkCommandPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = NULL;
    info.queueFamilyIndex = queue_family_index;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo GetCommandBufferAI(
            VkCommandPool command_pool, uint32_t count,
            VkCommandBufferLevel level
){
    VkCommandBufferAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = NULL;
    info.commandPool = command_pool;
    info.commandBufferCount = count;
    info.level = level;
    return info;
}

VkRenderPassCreateInfo GetRenderPassCI(
            uint32_t attachment_count,
            VkAttachmentDescription* attachments,
            uint32_t subpass_count,
            VkSubpassDescription* subpasses,
            uint32_t dependency_count,
            VkSubpassDependency* dependencies
) {
    VkRenderPassCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.attachmentCount = attachment_count;
    info.pAttachments = attachments;
    info.subpassCount = subpass_count;
    info.pSubpasses = subpasses;
    info.dependencyCount = dependency_count;
    info.pDependencies = dependencies;
    return info;
}

VkFenceCreateInfo GetFenceCI(VkFenceCreateFlags flags){
    VkFenceCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = flags;
    return info;
}

VkSemaphoreCreateInfo GetSemaphoreCI(VkSemaphoreCreateFlags flags){
    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = flags;
    return info;
}

VkCommandBufferBeginInfo GetCommandBufferBI(
            VkCommandBufferInheritanceInfo* inheritance_info,
            VkCommandBufferUsageFlags flags            
){
    VkCommandBufferBeginInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = NULL;
    info.pInheritanceInfo = inheritance_info,
    info.flags = flags;
    return info;
}

VkRenderPassBeginInfo GetRenderPassBI(
            VkRenderPass render_pass,
            VkOffset2D offset,
            VkExtent2D extent,
            VkFramebuffer framebuffer,
            uint32_t clear_value_count,
            VkClearValue* clear_values
){
    VkRenderPassBeginInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.pNext = NULL;
    info.renderPass = render_pass;
    info.renderArea.offset = offset;
    info.renderArea.extent = extent;
    info.framebuffer = framebuffer;
    info.clearValueCount = clear_value_count;
    info.pClearValues = clear_values;
    return info;
}

VkPipelineShaderStageCreateInfo GetShaderStageCI(VkShaderStageFlagBits stage_flags, VkShaderModule module){
    VkPipelineShaderStageCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.stage = stage_flags;
    info.module = module;
    info.pName = "main";
    info.pSpecializationInfo = NULL;
    return info;
}

VkPipelineVertexInputStateCreateInfo GetVertexInputCI(){
    VkPipelineVertexInputStateCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.vertexBindingDescriptionCount = 0;
    info.pVertexBindingDescriptions = NULL;
    info.vertexAttributeDescriptionCount = 0;
    info.pVertexAttributeDescriptions = NULL;
    return info;
}

VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyCI(VkPrimitiveTopology topology){
    VkPipelineInputAssemblyStateCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.topology = topology;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
}

VkPipelineRasterizationStateCreateInfo GetRasterizationCI(VkPolygonMode polygon_mode){
    VkPipelineRasterizationStateCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode = polygon_mode;
    info.lineWidth = 1.0f;
    info.cullMode = VK_CULL_MODE_NONE;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;
    return info;
}

VkPipelineMultisampleStateCreateInfo GetMultisampleCI(){
    VkPipelineMultisampleStateCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.sampleShadingEnable = VK_FALSE;
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading = 1.0f;
    info.pSampleMask = NULL;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
}

VkPipelineColorBlendAttachmentState GetColorBlendAttachmentState(){
    VkPipelineColorBlendAttachmentState attachment = {0};
    attachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = VK_FALSE;
    return attachment;
}

VkPipelineLayoutCreateInfo GetPipelineLayoutCI(){
    VkPipelineLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = NULL;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = NULL;
    return info;
}
