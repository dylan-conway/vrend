#ifndef _VREND_DEBUG_H_
#define _VREND_DEBUG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

VkBool32 CheckInstanceLayers();
void DebugUtilsInstanceLayers(VkInstanceCreateInfo* ci);
void InitDebugUtils(VkInstance* vk_instance);
void FreeDebugUtils(VkInstance* vk_instance);

VKAPI_ATTR VkBool32 VKAPI_CALL _DebugCallback();
PFN_vkCreateDebugUtilsMessengerEXT _CreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT _DestroyDebugUtilsMessengerEXT;

#endif