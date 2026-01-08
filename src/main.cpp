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
#include <random>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
uint8_t memory[4096];
uint16_t stack[16];
uint8_t sp = 0;
uint16_t pc, i_reg;
uint8_t delay_reg, sound_reg;
uint8_t vars[16];
uint8_t display[32][64];
static SDL_Texture *fb_tex = nullptr;
constexpr uint16_t FONT_BASE = 0x050;
SDL_Event event;
std::random_device rd;
std::mt19937 gen(rd());
int8_t latched_key = -1;
uint64_t last_timer_tick = 0;
double timer_accumulator = 0.0;
uint8_t temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;

std::uniform_int_distribution<uint8_t> dist(0, 255);

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

static const uint8_t keys[16] = {
    27, // chip8: 0, keyboard: x
    30, // chip8: 1, keyboard: 1
    31, // chip8: 2, keyboard: 2
    32, // chip8: 3, keyboard: 3
    20, // chip8: 4, keyboard: q
    26, // chip8: 5, keyboard: w
    8,  // chip8: 6, keyboard: e
    4,  // chip8: 7, keyboard: a
    22, // chip8: 8, keyboard: s
    7,  // chip8: 9, keyboard: d
    29, // chip8: a, keyboard: z
    6,  // chip8: b, keyboard: c
    33, // chip8: c, keyboard: 4
    21, // chip8: d, keyboard: r
    9,  // chip8: e, keyboard: f
    25  // chip8: f, keyboard: v
};

uint8_t chip8_keys[16];

bool loadROM(const char *path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > (4096 - 0x200))
        return false;

    file.read((char *)&memory[0x200], size);
    return true;
}

void printHex(uint16_t value)
{
    std::cout << std::hex << std::setw(4) << std::setfill('0') << value << "\n";
}

void clearScreen()
{
    // printf("Clearing screen\n");
    memset(display, 0, sizeof(display));
}

void updateDisplay(uint8_t x, uint8_t y, uint8_t n)
{
    uint8_t x_pixel = vars[x] % 64;
    uint8_t y_pixel = vars[y] % 32;
    // printf("Drawing sprite: V%x=%d V%y=%d n=%d -> x_pixel=%d y_pixel=%d i_reg=0x%x VF_before=%d\n", x, vars[x], y, vars[y], n, x_pixel, y_pixel, i_reg, vars[0xF]);
    vars[0xf] = 0;
    uint16_t addr = i_reg;
    for (int i = y_pixel; i < y_pixel + n; i++)
    {
        if (i >= 32)
            break;
        for (int j = x_pixel; j < x_pixel + 8; j++)
        {
            if (j >= 64)
                break;
            uint8_t sprite_bit = (((memory[addr] >> (7 - (j - x_pixel)))) & 0x01);
            vars[0xF] |= (sprite_bit && display[i][j]);
            display[i][j] ^= sprite_bit;
        }
        addr += 1;
    }
    // printf("  After draw: VF=%d\n", vars[0xF]);
}

void skipKey(uint8_t reg, uint16_t NN)
{
    if (NN == 0x9E)
    {
        if (chip8_keys[vars[reg] & 0xF])
        {
            pc += 2;
        }
    }
    else if (NN == 0xA1)
    {
        if (!chip8_keys[vars[reg] & 0xF])
        {
            pc += 2;
        }
    }
}
void step()
{
    // fetch
    uint16_t instr = (memory[pc] << 8) | memory[pc + 1];
    uint16_t pc_before = pc;
    // printf("Printing instruction fetched from memory at location %x: ", pc);
    pc += 2;

    // printHex(instr);

    // decode/execute
    uint8_t nibble_1 = (instr >> 12) & 0xF;
    uint8_t nibble_2 = (instr >> 8) & 0xF;
    uint8_t nibble_3 = (instr >> 4) & 0xF;
    uint8_t nibble_4 = instr & 0xF;
    uint16_t NN = instr & 0xFF;
    uint16_t NNN = instr & 0xFFF;

    switch (nibble_1)
    {
    case 0x0:
        if (instr == 0x00E0)
        {
            clearScreen();
        }
        else if (instr == 0x00EE)
        {
            // printf("Returning from subroutine\n");
            if (sp > 0)
            {
                pc = stack[--sp];
            }
            else
            {
                // printf("Stack underflow\n");
            }
        }
        break;
    case 0x1: // 0x1nnn
        // printf("Setting pc to %x\n", NNN);
        pc = NNN;
        break;
    case 0x2: // 0x2nnn
        // printf("Calling subroutine\n");
        if (sp < 16)
        {
            stack[sp++] = pc;
        }
        else
        {
            // printf("Stack overflow\n");
        }
        pc = NNN;
        break;
    case 0x3: // 0x3xnn
        // printf("Skip if V%x (%d) == %x: ", nibble_2, vars[nibble_2], NN);
        if (vars[nibble_2] == NN)
        {
            // printf("YES - skipping\n");
            pc += 2;
        }
        else
        {
            // printf("NO\n");
        }
        break;
    case 0x4: // 0x4xnn
        // printf("Skip if V%x (%d) != %x: ", nibble_2, vars[nibble_2], NN);
        if (vars[nibble_2] != NN)
        {
            // printf("YES - skipping. Next instr would be at 0x%x: ", pc);
            uint16_t next_instr = (memory[pc] << 8) | memory[pc + 1];
            // printHex(next_instr);
            pc += 2;
        }
        else
        {
            // printf("NO\n");
        }
        break;
    case 0x5:
        // printf("Skip if V%x (%d) == V%x (%d): ", nibble_2, vars[nibble_2], nibble_3, vars[nibble_3]);
        if (nibble_4 == 0x0)
        { // 0x5XY0
            if (vars[nibble_2] == vars[nibble_3])
            {
                // printf("YES - skipping\n");
                pc += 2;
            }
            else
            {
                // printf("NO\n");
            }
        }
        break;
    case 0x6: // 0x6xnn
        // printf("Setting register %x to %x\n", nibble_2, NN);
        vars[nibble_2] = NN;
        break;
    case 0x7: // 0x7xnn
        // printf("Adding %x to register %x\n", NN, nibble_2);
        vars[nibble_2] += NN;
        if (nibble_2 <= 7 || nibble_2 == 0xF)
        {
            // printf("  V%x += 0x%x -> V%x now = %d\n", nibble_2, NN, nibble_2, vars[nibble_2]);
        }
        break;
    case 0x8: // logical and arithmetic instructions for 0x8XY_
        switch (nibble_4)
        {
        case 0:
            // printf("Set Vx to the value of Vy\n");
            vars[nibble_2] = vars[nibble_3];
            break;
        case 1:
            // printf("Set Vx to the bitwise OR of VX and VY\n");
            vars[nibble_2] = vars[nibble_2] | vars[nibble_3];
            break;
        case 2:
            // printf("Set Vx to the bitwise AND of Vx and Vy\n");
            vars[nibble_2] = vars[nibble_2] & vars[nibble_3];
            break;
        case 3:
            // printf("Set Vx to the bitwise XOR of Vx and Vy\n");
            vars[nibble_2] = vars[nibble_2] ^ vars[nibble_3];
            break;
        case 4:
        {
            // printf("Vx is set to the value of Vx + Vy\n");
            uint16_t sum = vars[nibble_2] + vars[nibble_3];
            vars[0xf] = (sum > 255);
            if (nibble_2 != 0xF)
            {
                vars[nibble_2] = sum & 0xFF;
            }
            break;
        }
        case 5:
            // printf("Vx is set to the value of Vx - Vy\n");
            vars[0xf] = (vars[nibble_2] >= vars[nibble_3]);
            if (nibble_2 != 0xF)
            {
                vars[nibble_2] = vars[nibble_2] - vars[nibble_3];
            }
            break;
        case 7:
            // printf("Vx is set to the value of Vy - Vx\n");
            if (nibble_2 != 0xF)
            {
                vars[0xf] = (vars[nibble_3] >= vars[nibble_2]);
                vars[nibble_2] = vars[nibble_3] - vars[nibble_2];
            }
            else
            {
                // If Vx is VF, just set VF to the borrow bit
                vars[0xf] = (vars[nibble_3] >= vars[nibble_2]);
            }
            break;
        case 6:
            // printf("Shift Vx to the right by 1\n");
            vars[0xf] = vars[nibble_2] & 0x1;
            if (nibble_2 != 0xF)
            {
                vars[nibble_2] >>= 1;
            }
            break;
        case 0xE:
            // printf("Shift Vx to the left by 1\n");
            vars[0xf] = (vars[nibble_2] >> 7) & 0x1;
            if (nibble_2 != 0xF)
            {
                vars[nibble_2] <<= 1;
            }
            break;
        default:
            // printf("Unknown opcode: ");
            // printHex(instr);
            break;
        }
        break;
    case 0x9: // 0x9xy0
        // printf("Skip if V%x (%d) != V%x (%d): ", nibble_2, vars[nibble_2], nibble_3, vars[nibble_3]);
        if (vars[nibble_2] != vars[nibble_3])
        {
            // printf("YES - skipping\n");
            pc += 2;
        }
        else
        {
            // printf("NO\n");
        }
        break;
    case 0xA: // 0xANNN
        // printf("Setting index register to %x\n", NNN);
        i_reg = NNN;
        break;
    case 0xB: // 0xBNNN
        // printf("Jumping to addr NNN + value in reg v0\n");
        pc = NNN + vars[0];
        break;
    case 0xC:
    { // 0xCXNN
        // printf("Storing a random value in Vx\n");
        vars[nibble_2] = dist(gen) & NN;
        break;
    }
    case 0xD: // 0xDXYN
        // display/draw
        updateDisplay(nibble_2, nibble_3, nibble_4);
        break;
    case 0xE:
        // printf("Skip instr based on key pressed\n");
        skipKey(nibble_2, NN);
        break;
    case 0xF:
        if (NN == 0x07)
        {
            // printf("Setting register %d to value of delay reg (%x)\n", nibble_2, delay_reg);
            vars[nibble_2] = delay_reg;
        }
        else if (NN == 0x15)
        {
            // printf("Setting delay reg to value %x\n", vars[nibble_2]);
            delay_reg = vars[nibble_2];
        }
        else if (NN == 0x18)
        {
            // printf("Setting sound reg to value %x\n", vars[nibble_2]);
            sound_reg = vars[nibble_2];
        }
        else if (NN == 0x1E)
        {
            // printf("Add Vx to I\n");
            i_reg += vars[nibble_2];
            vars[0xF] = (i_reg > 0x0FFF);
        }
        else if (NN == 0x0A)
        {
            if (latched_key == -1)
            {
                // printf("Undo PC increment\n");
                pc -= 2; // undo pre-increment
                return;
            }
            else
            {
                // printf("Write key to register and continue program\n");
                vars[nibble_2] = latched_key; // CPU writes register
                latched_key = -1;             // consume input
                return;
            }
        }
        else if (NN == 0x29)
        {
            // printf("Setting index register to address of hex value stored in register %d\n", nibble_2);
            i_reg = vars[nibble_2] * 5 + FONT_BASE;
        }
        else if (NN == 0x33)
        {
            // printf("Storing decimal places of %d in memory\n", vars[nibble_2]);
            temp1 = vars[nibble_2] / 100;
            temp2 = (vars[nibble_2] / 10) % 10;
            temp3 = vars[nibble_2] % 10;
            memory[i_reg] = temp1;
            memory[i_reg + 1] = temp2;
            memory[i_reg + 2] = temp3;
        }
        else if (NN == 0x55)
        {
            for (temp0 = 0; temp0 <= nibble_2; temp0++)
            {
                // printf("Storing register %d in memory\n", temp0);
                memory[i_reg + temp0] = vars[temp0];
            }
        }
        else if (NN == 0x65)
        {
            for (temp0 = 0; temp0 <= nibble_2; temp0++)
            {
                // printf("Loading memory into register %d\n", temp0);
                vars[temp0] = memory[i_reg + temp0];
            }
        }
        else
        {
            printf("Unknown opcode: ");
            // printHex(instr);
        }
        break;
    default:
        printf("Unknown opcode: ");
        // printHex(instr);
        break;
    }
}

void writeToMemory(uint16_t instr)
{
    uint8_t upper = (instr >> 8) & 0xFF;
    uint8_t lower = instr & 0xFF;

    memory[pc] = upper;
    memory[pc + 1] = lower;
    pc += 2;
}

void loadFonts()
{
    for (int i = 0; i < 80; i++)
    {
        memory[FONT_BASE + i] = font[i];
    }
}

void uploadFrameBuffer()
{
    // Scale factor: 10x (640/64 = 10, 320/32 = 10)
    const int SCALE = 10;
    
    uint32_t *pixels;
    int pitch;

    SDL_LockTexture(fb_tex, nullptr, (void **)&pixels, &pitch);

    int row_pixels = pitch / sizeof(uint32_t);

    // For each original pixel, write a SCALE x SCALE block
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 64; x++)
        {
            uint32_t color = display[y][x] ? 0xFFFFFFFF : 0xFF000000;
            
            // Fill SCALE x SCALE block for this pixel
            for (int sy = 0; sy < SCALE; sy++)
            {
                for (int sx = 0; sx < SCALE; sx++)
                {
                    int pixel_x = x * SCALE + sx;
                    int pixel_y = y * SCALE + sy;
                    pixels[pixel_y * row_pixels + pixel_x] = color;
                }
            }
        }
    }

    SDL_UnlockTexture(fb_tex);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (argc != 2) {
        SDL_Log("Usage: %s <path_to_rom>", argv[0]);
        return SDL_APP_FAILURE;
    }

    memset(memory, 0, sizeof(memory));
    memset(vars, 0, sizeof(vars));
    memset(stack, 0, sizeof(stack));
    memset(display, 0, sizeof(display));
    memset(chip8_keys, 0, sizeof(chip8_keys));
    delay_reg = sound_reg = 0;
    last_timer_tick = SDL_GetTicks();
    timer_accumulator = 0.0;

    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("CHIP-8 Emulator", 640, 320, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("Couldn't create a window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 64, 32, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    fb_tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        640, 320);

    if (!fb_tex)
    {
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
    if (!loadROM(argv[1]))
    {
        SDL_Log("Failed to load ROM");
        return SDL_APP_FAILURE;
    }
    pc = 0x200;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{

    // while (SDL_PollEvent(&event)) {
    //     if (event.type == SDL_EVENT_QUIT) {
    //         return SDL_APP_SUCCESS;
    //     }

    //     if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
    //         if (event.key.repeat) continue;

    //         SDL_Scancode sc = event.key.scancode;
    //         uint8_t pressed = (event.type == SDL_EVENT_KEY_DOWN);

    //         for (int k = 0; k < 16; k++) {
    //             if (keys[k] == sc) {
    //                 chip8_keys[k] = pressed;
    //                 if (pressed && latched_key == -1) {
    //                     latched_key = k;
    //                 }
    //                 break;
    //             }
    //         }
    //     }
    // }

    const float red = 0;
    const float green = 0;
    const float blue = 0;
    SDL_SetRenderDrawColor(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);

    uint64_t now_ms = SDL_GetTicks();
    double delta_ms = now_ms - last_timer_tick;
    last_timer_tick = now_ms;

    timer_accumulator += delta_ms;

    const double TIMER_INTERVAL_MS = 1000.0 / 60.0;

    while (timer_accumulator >= TIMER_INTERVAL_MS)
    {
        if (delay_reg > 0)
            delay_reg--;
        if (sound_reg > 0)
            sound_reg--;
        timer_accumulator -= TIMER_INTERVAL_MS;
    }

    for (int k = 0; k < 3; k++)
    {
        step();
    }
    uploadFrameBuffer();
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, fb_tex, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT ||
        event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
    {
        if (event->key.repeat)
            return SDL_APP_CONTINUE;

        SDL_Scancode sc = event->key.scancode;
        uint8_t pressed = (event->type == SDL_EVENT_KEY_DOWN);

        for (int k = 0; k < 16; k++)
        {
            if (keys[k] == sc)
            {
                chip8_keys[k] = pressed;
                if (pressed && latched_key == -1)
                {
                    latched_key = k;
                }
                break;
            }
        }
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}

// int main() {

//     return 0;
// }
