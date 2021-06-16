#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "vrend.h"

SDL_bool running;

int main(int argc, char** argv){

    running = SDL_TRUE;

    INIT_VREND("Vulkan CA", 640, 480);

    // PrintVulkanInstance();

    SDL_Event event;
    while(running){
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    running = SDL_FALSE;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym){
                        case SDLK_ESCAPE:
                            running = SDL_FALSE;
                            break;
                    }
            }
        }

        // DrawFrame();
    }

    FREE_VREND();
    return 0;
}
