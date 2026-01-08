#include <iostream>
#include <iomanip>
#include <cstdint>
#include <thread>
#include <chrono>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <fstream>
#include <cstring>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
uint8_t memory[4096];
uint16_t stack[16];
uint16_t pc, i_reg;
uint8_t delay_reg, sound_reg;
uint8_t vars[16]; 
uint8_t display[32][64];
static SDL_Texture* fb_tex = nullptr;
constexpr uint16_t FONT_BASE = 0x050;

static const uint8_t font[80] = {
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

bool loadROM(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > (4096 - 0x200)) return false;

    file.read((char*)&memory[0x200], size);
    return true;
}

void printHex(uint16_t value) {
    std::cout << std::hex << std::setw(4) << std::setfill('0') << value << "\n";
}

void clearScreen() {
    printf("Clearing screen\n");
    memset(display, 0, sizeof(display));
}

void updateDisplay(uint8_t x, uint8_t y, uint8_t n) {
    printf("Drawing to display\n");
    uint8_t x_pixel = vars[x] % 64;
    uint8_t y_pixel = vars[y] % 32;
    vars[0xf] = 0;
    uint16_t addr = i_reg;
    for (int i = y_pixel; i < y_pixel + n; i++) {
        if (i >= 32) break;
        for (int j = x_pixel; j < x_pixel + 8; j++) {
            if (j >= 64) break;
            uint8_t sprite_bit = (((memory[addr] >> (7 - (j - x_pixel)))) & 0x01);            
            vars[0xF] |= (sprite_bit && display[i][j]);
            display[i][j] ^= sprite_bit;
        }
        addr += 1;
    }
}

void step() {
    // fetch
    uint16_t instr = (memory[pc] << 8) | memory[pc + 1];
    printf("Printing instruction fetched from memory at location %x: ", pc);
    pc += 2;
    
    printHex(instr);

    // decode/execute
    uint8_t nibble_1 = (instr >> 12) & 0xF;
    uint8_t nibble_2 = (instr >> 8) & 0xF;
    uint8_t nibble_3 = (instr >> 4) & 0xF;
    uint8_t nibble_4 = instr & 0xF;
    uint16_t NN = instr & 0xFF;
    uint16_t NNN = instr & 0xFFF;

    switch (nibble_1) {
        case 0x0:
            if (instr == 0x00E0) {
                clearScreen();
            }
            break;
        case 0x1: // 0x1nnn
            printf("Setting pc to %x\n", NNN);
            pc = NNN;
            break;
        case 0x6: // 0x6xnn
            printf("Setting register %x to %x\n", nibble_2, NN);
            vars[nibble_2] = NN;
            break;
        case 0x7: // 0x7xnn
            printf("Adding %x to register %x\n", NN, nibble_2);
            vars[nibble_2] += NN;
            break;
        case 0xA: // 0xANNN
            printf("Setting index register to %x\n", NNN);
            i_reg = NNN;
            break;
        case 0xD: // 0xDXYN
            // display/draw
            updateDisplay(nibble_2, nibble_3, nibble_4);
            break;
        default: 
            printf("Unknown opcode: ");
            printHex(instr);
            break;
    }
}

void writeToMemory(uint16_t instr) {
    uint8_t upper = (instr >> 8) & 0xFF;
    uint8_t lower = instr & 0xFF;

    memory[pc] = upper;
    memory[pc + 1] = lower;
    pc += 2;
}

void loadFonts() {
    for (int i = 0; i < 80; i++) {
        memory[FONT_BASE + i] = font[i];
    }
}

void uploadFrameBuffer() {
    uint32_t* pixels;
    int pitch;

    SDL_LockTexture(fb_tex, nullptr, (void**)&pixels, &pitch);

    int row_pixels = pitch / sizeof(uint32_t);

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            pixels[y * row_pixels + x] =
                display[y][x] ? 0xFFFFFFFF : 0xFF000000;
        }
    }

    SDL_UnlockTexture(fb_tex);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    memset(memory, 0, sizeof(memory));
    memset(vars, 0, sizeof(vars));
    memset(stack, 0, sizeof(stack));
    memset(display, 0, sizeof(display));
    delay_reg = sound_reg = 0;
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", 64, 32, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create a window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 64, 32, SDL_LOGICAL_PRESENTATION_LETTERBOX);


    fb_tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        64, 32
    );

    if (!fb_tex) {
        SDL_Log("Texture create failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    loadFonts();
    // writeToMemory(0x00E0);
    // writeToMemory(0x6020);
    // writeToMemory(0x6110);
    // writeToMemory(0xA05f);
    // writeToMemory(0xD015);
    // writeToMemory(0x120a);
    
    // pc = 0x200;
    if (!loadROM("examples/ibm_logo.ch8")) {
        SDL_Log("Failed to load ROM");
        return SDL_APP_FAILURE;
    }
    pc = 0x200;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {



    const double now = ((double)SDL_GetTicks()) / 1000.0;
    const float red = 0;
    const float green = 0;
    const float blue = 0;
    SDL_SetRenderDrawColor(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);

    for (int k = 0; k < 20; k++) {
        step();
    }    
    uploadFrameBuffer();
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, fb_tex, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}



// int main() {


//     return 0;
// }

