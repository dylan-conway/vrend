#ifndef _VKINIT_H_
#define _VKINIT_H_

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

VkCommandPoolCreateInfo GetCommandPoolCI(
    uint32_t queue_family_index,
    VkCommandPoolCreateFlags flags
);

VkCommandBufferAllocateInfo GetCommandBufferAI(
    VkCommandPool command_pool,
    uint32_t count,
    VkCommandBufferLevel level
);

VkRenderPassCreateInfo GetRenderPassCI(
    uint32_t attachment_count,
    VkAttachmentDescription* attachments,
    uint32_t subpass_count,
    VkSubpassDescription* subpasses,
    uint32_t dependency_count,
    VkSubpassDependency* dependencies
);

VkFenceCreateInfo GetFenceCI(
    VkFenceCreateFlags flags
);

VkSemaphoreCreateInfo GetSemaphoreCI(
    VkSemaphoreCreateFlags flags
);

VkCommandBufferBeginInfo GetCommandBufferBI(
    VkCommandBufferInheritanceInfo* inheritance_info,
    VkCommandBufferUsageFlags flags            
);

VkRenderPassBeginInfo GetRenderPassBI(
    VkRenderPass render_pass,
    VkOffset2D offset,
    VkExtent2D extent,
    VkFramebuffer framebuffer,
    uint32_t clear_value_count,
    VkClearValue* clear_values
);

#endif