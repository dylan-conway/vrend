#ifndef _VREND_H_
#define _VREND_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "vk_struct_init.h"
#include "vk_enum_str.h"

#ifdef DEBUG
    #include "vrend_debug.h"
#endif

#define STR(x) #x

// Check result from Vulkan function. Will print on exit and on success with debug
#define VK_CHECK(fname, ...) CHECK(fname(__VA_ARGS__), STR(fname), VK_TRUE)

// Silent check result from Vulkan function. Only prints on exit.
#define VK_CHECK_S(fname, ...) CHECK(fname(__VA_ARGS__), STR(fname), VK_FALSE);

void INIT_VREND(char* title, uint32_t w, uint32_t h);
void FREE_VREND();
void DRAW_VREND();

#endif