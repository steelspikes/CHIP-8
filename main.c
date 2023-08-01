#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    uint32_t fg_color;
    uint32_t bg_color;
} config_t;

bool init_SDL(sdl_t *sdl, const config_t config) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL sybsystems! ", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.window_width, config.window_height, 0);

    if(!sdl->window) {
        SDL_Log("Could not create window ", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);

    if(!sdl->renderer) {
        SDL_Log("Could not create SDL renderer ", SDL_GetError());
    }

    return true;
}

void final_cleanup(const sdl_t sdl) {
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit();
}

bool set_config_from_args(config_t *config, int argc, char **argv) {
    *config = (config_t) {
        .window_width = 64,
        .window_height = 32,
        .fg_color = 0xFFFF00FF,
        .bg_color = 0x00000000,
    };

    for(int i = 1; i < argc; i++) {}

    return true;
}

void clear_screen(const config_t config, const sdl_t sdl) {
    const uint8_t r = (config.bg_color >> 24) & 0xFF;
    const uint8_t g = (config.bg_color >> 16) & 0xFF;
    const uint8_t b = (config.bg_color >> 8) & 0xFF;
    const uint8_t a = (config.bg_color >> 0) & 0xFF;
    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

int main(int argc, char **argv) {
    //Initialize emulator configuration options
    config_t config = {0};
    if(!set_config_from_args(&config, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    sdl_t sdl = {0};
    //Initialize SDL
    if(!init_SDL(&sdl, config)) {
        exit(EXIT_FAILURE);
    }

    //Initial screen clear
    clear_screen(config, sdl);

    //Main emulator loop
    while(true) {
        //Delay for 60hz/60fps
        // SDL_Delay();

        //Update window with changes
        // update_display(config);
    }

    //Final cleanup
    final_cleanup(sdl);

    exit(EXIT_SUCCESS);
}