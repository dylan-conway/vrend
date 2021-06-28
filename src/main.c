#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "vrend.h"

SDL_bool running;

double GetDeltaTime(Uint64 start, Uint64 finish);

int main(int argc, char** argv){

    running = SDL_TRUE;

    INIT_VREND("Vulkan CA", 640, 480);

    SDL_Event event;
    while(running){
        Uint64 start_time = SDL_GetPerformanceCounter();
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

        DRAW_VREND();

        Uint64 finish_time = SDL_GetPerformanceCounter();
        while(GetDeltaTime(start_time, finish_time) < 16.667){
            finish_time = SDL_GetPerformanceCounter();
        }
    }

    FREE_VREND();
    return 0;
}

double GetDeltaTime(Uint64 start, Uint64 finish){
    return (double)(finish - start) / SDL_GetPerformanceFrequency() * 1000;
}
