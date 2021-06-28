
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
