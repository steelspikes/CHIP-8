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
    uint32_t scale_factor;
} config_t;

//States of emulator
typedef enum {
    QUIT,
    RUNNING,
    PAUSED
} emulator_state_t;

typedef struct {
    uint16_t opcode;
    uint16_t NNN; //12 bit address/constant
    uint8_t NN; //8 bit constant
    uint8_t N;  //4 bit constant
    uint8_t X;  //4 bit register identifier
    uint8_t Y;  //4 bit register identifier  
} instruction_t;

//CHIP8 machine object
typedef struct {
    emulator_state_t state;
    uint8_t ram[4096];
    bool display[64 * 32]; //Emulate original CHIP8 resolution pixels
    uint16_t stack[12]; //Subroutine stack
    uint16_t *stack_ptr;
    uint8_t V[16]; //Data registers V0-VF
    uint16_t I; //Index register
    uint16_t PC; //Program counter
    uint16_t delay_timer; //Decrements at 60hz when > 0
    uint16_t sound_timer; //Decrements at 60hz and plays tone when > 0
    bool keypad[16]; //Hexadecimal keypad 0x0-0xF
    const char *rom_name; //Currently running ROM
    instruction_t inst; //Currently executing instruction
} chip8_t;

bool init_SDL(sdl_t *sdl, const config_t config) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL sybsystems! ", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow(
        "Chip-8 Emulator", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        config.window_width * config.scale_factor, 
        config.window_height * config.scale_factor, 
        0
    );

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
        .fg_color = 0xFFFFFFFF,
        .bg_color = 0x00000000,
        .scale_factor = 20,
    };

    for(int i = 1; i < argc; i++) {}

    return true;
}

void clear_screen(const sdl_t sdl, const config_t config) {
    const uint8_t r = (config.bg_color >> 24) & 0xFF;
    const uint8_t g = (config.bg_color >> 16) & 0xFF;
    const uint8_t b = (config.bg_color >> 8) & 0xFF;
    const uint8_t a = (config.bg_color >> 0) & 0xFF;
    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

void update_screen(const sdl_t sdl, const chip8_t chip8) {
    // SDL_Rect
    SDL_RenderPresent(sdl.renderer);
}

void handle_input(chip8_t *chip8) {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT: {
                //End program
                chip8->state = QUIT;
                return;
            }

            case  SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: {
                        //Scape key
                        chip8->state = QUIT;
                        return;
                    }

                    case SDLK_SPACE: {
                        //Space bar
                        if(chip8->state == RUNNING) {
                            chip8->state = PAUSED;
                        } else {
                            chip8->state = RUNNING;
                            puts("PAUSED");
                        }
                    }
                }

                break;
            }

            case SDL_KEYUP: {
                break;
            }

            default: {
                break;
            }
        }
    }
}

bool init_chip8(chip8_t *chip8, const char rom_name[]) {
    const uint32_t entry_point = 0x200; //CHIP8 ROMS will loaded to 0x200
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    //Load font
    memcpy(&chip8->ram[0], font, sizeof(font));

    //Load ROM
    FILE *rom = fopen(rom_name, "rb");
    if(!rom) {
        SDL_Log("ROM file %s is invalid or not exists", rom_name);
        return false;
    }
    fseek(rom, 0, SEEK_END);
    const long rom_size = ftell(rom);
    const long max_size = sizeof chip8->ram - entry_point;
    rewind(rom);

    if(rom_size > max_size) {
        SDL_Log("ROM file %s is too big! ROM size: %zu, Max size allowed: %zu", rom_name, rom_size, max_size);
    }

    if(fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1) {
        SDL_Log("Could not read ROM file");
        return false;
    }
    fclose(rom);

    //Set chip8 machine defaults
    chip8->state = RUNNING;
    chip8->PC = entry_point;
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0];


    return true;
}

#ifdef DEBUG
void print_debug_info(chip8_t *chip8) {
    printf("Address: 0x%04X, Opcode: 0x%04X Desc: ", chip8->PC - 2, chip8->inst.opcode);

    switch((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x00: {
            if(chip8->inst.NN == 0xE0){
                printf("Clean screen");
                puts("");
            } else if(chip8->inst.NN == 0xEE) {
                printf("Return from subroutine to address 0x%04x", *(chip8->stack_ptr - 1));
                puts("");
            } else {
                puts("");
            }

            break;
        }

        case 0x02: {
            *chip8->stack_ptr++ == chip8->PC;
            chip8->PC = chip8->inst.NNN;
            break;
        }

        case 0x06:{
            printf("Set register V%X to NN (0x%02X)",chip8->inst.X, chip8->inst.NN);
            puts("");
        }

        case 0x0A: {
            printf("Set I to NNN (0x%04X)", chip8->inst.NNN);
            puts("");
            break;
        }

        default: {
            printf("Unimplemented opcode");
            puts("");
            break;
        }
    }
}
#endif

void emulate_instruction(chip8_t *chip8, const config_t *config) {
    //Get next opcode from ram
    printf("        Binary code: %02X-%02X", chip8->ram[chip8->PC], chip8->ram[chip8->PC+1]);
    puts("");
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC+1];
    chip8->PC += 2; //Pre increment program counter for next opcode

    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x0FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

    // printf("Mira 0x%04X y PC es %i", chip8->inst.opcode, chip8->PC);
    // puts("");

#ifdef DEBUG
    print_debug_info(chip8);
#endif

    //Emulate opcode
    switch((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x00: {
            if(chip8->inst.NN == 0xE0) {
                //0x00E0 Clear screen
                memset(&chip8->display[0], false, sizeof chip8->display);
            } else if(chip8->inst.NN == 0xEE) {
                //0x00EE Return from subroutine
                //Set program counter to last address on subroutine stack ("pop" it from stack), so next opcode will be gotten from that address
                chip8->PC = *(--chip8)->stack;
            }

            break;
        }

        case 0x02: {
            //0x2NNN Call subroutine at NNN
            *chip8->stack_ptr = chip8->PC; //Store current address to return on a subroutine stack and set program counter to subroutine address that the next opcode is gotten from there
            chip8->PC = chip8->inst.NNN;
            break;
        }

        case 0x06:{
            // Set register VX to NN
            chip8->V[chip8->inst.X] = chip8->inst.NN;
            break;
        }

        case 0x0A: {
            //Set index register I to NNN
            chip8->I = chip8->inst.NNN;
            break;
        }

        case 0x0D: {
            //Draw N height sprite at coords x,y
            //Screen pixels are XOR with sprite bits
            //VF carry flag is set if any screen pixels are set off. Useful for collisions 
            uint8_t x_coord = chip8->V[chip8->inst.X] % config->window_width;
            uint8_t y_coord = chip8->V[chip8->inst.Y] % config->window_height;
            chip8->V[0xF] = 0; //Initialize carry flag 0
            for(uint8_t i = 0; chip8->inst.N; i++) { //Loop over all N rows of sprite
                const uint8_t sprite_data = chip8->ram[chip8->I + i];
                x_coord = orig_x;

                for(uint8_t j = 7; j >= 0; j--) {
                    bool *pixel = &chip8->display[y_coord * config->window_height + x_coord];
                    const bool sprite_bit = sprite_data & (1 << j);
                    //If sprite pixel or bit is on and display pixels is on, set carry flag
                    if(sprite_bit && *pixel) {
                        chip8->V[0xF] = 1;
                    }
                    
                    //XOR display pixel with sprote pixel/bit to set it on or off
                    *pixel ^= sprite_bit;

                    //Stop drawing if hit right edge of screen
                    if(++x_coord >= config->window_width) break;
                }

                if(++y_coord >= config->window_height) break;
            }
            break;
        }

        default: {
            break;
        }
    }
}

int main(int argc, char **argv) {
    //Initialize emulator configuration options
    config_t config;
    if(!set_config_from_args(&config, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    sdl_t sdl;
    //Initialize SDL
    if(!init_SDL(&sdl, config)) {
        exit(EXIT_FAILURE);
    }

    //Initialize CHIP8 machine
    chip8_t chip8;
    char *rom_name = argv[1];
    if(!init_chip8(&chip8, rom_name)) {
        exit(EXIT_FAILURE);
    }

    //Initial screen clear
    clear_screen(sdl, config);

    //Main emulator loop
    while(chip8.state != QUIT) {
        //Handle user input
        handle_input(&chip8);

        if(chip8.state == PAUSED) continue;

        //Emulate CHIP8 instructions
        emulate_instruction(&chip8, &config);

        //Delay for 60hz/60fps (16.67ms)
        SDL_Delay(16);

        //Update window with changes
        update_screen(sdl, chip8);
    }

    //Final cleanup
    final_cleanup(sdl);

    exit(EXIT_SUCCESS);
}