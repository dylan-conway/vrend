
BUILD_MODE ?= DEBUG

target = main
cc = gcc
flags = -std=c99 -Wall
include_paths = -I"C:/VulkanSDK/1.2.176.1/Include" -I"C:/mingw64/mingw64/include"
library_paths = -L"C:/VulkanSDK/1.2.176.1/Lib" -L"C:/mingw64/mingw64/lib"
libraries = -lmingw32 -lSDL2main -lSDL2 -lvulkan-1
src = src/main.c src/vrend.c

ifeq ($(BUILD_MODE), RELEASE)
	flags += -O3
endif
ifeq ($(BUILD_MODE), DEBUG)
	flags += -g -O3 -DDEBUG
	src += src/vrend_debug.c
endif

all: $(src)
	$(cc) $(flags) $(include_paths) -o $(target) $(src) $(library_paths) $(libraries)


.PHONY: clean run

run:
	./$(target)

clean:
	
