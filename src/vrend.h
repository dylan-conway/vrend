#ifndef _VREND_H_
#define _VREND_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "vrend_debug.h"

void INIT_VREND(char* title, uint32_t w, uint32_t h);
void FREE_VREND();

#endif