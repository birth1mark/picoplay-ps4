
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <fstream>
#include <iostream>
using namespace std;

#include "sdl2basehost.h"
#include "../../../source/hostVmShared.h"
#include "../../../source/nibblehelpers.h"
#include "../../../source/logger.h"
#include "../../../source/filehelpers.h"

// sdl
#include <SDL2/SDL.h>

#define SAMPLERATE 22050
#define SAMPLESPERBUF (SAMPLERATE / 30)
#define NUM_BUFFERS 2

int _windowWidth = 128;
int _windowHeight = 128;


int _screenWidth = 128;
int _screenHeight = 128;

int _maxNoStretchWidth = 128;
int _maxNoStretchHeight = 128;

const int PicoScreenWidth = 128;
const int PicoScreenHeight = 128;

uint32_t _windowFlags;
uint32_t _rendererFlags;
uint32_t _pixelFormat;

uint32_t last_time;
uint32_t now_time;
uint32_t frame_time;
uint32_t targetFrameTimeMs;

uint8_t currKDown;
uint8_t currKHeld;
bool stretchKeyPressed = false;

Audio* _audio;

SDL_Window* window;
SDL_Renderer *renderer;
SDL_Texture *texture = NULL;
SDL_AudioSpec want, have;
SDL_AudioDeviceID dev;
void *pixels;
uint8_t *base;
int pitch;

SDL_Rect DestR;
SDL_Rect SrcR;
double textureAngle;
SDL_RendererFlip flip;

int joystickCount = 0;

bool audioInitialized = false;


void postFlipFunction(){
    // We're done rendering, so we end the frame here.
    SDL_UnlockTexture(texture);
    if (textureAngle == 0 && flip == SDL_FLIP_NONE) {
        SDL_RenderCopy(renderer, texture, &SrcR, &DestR);
    } else {
        SDL_RenderCopyEx(renderer, texture, &SrcR, &DestR, textureAngle, NULL, flip);
    }

    SDL_RenderPresent(renderer);
}

void audioCleanup(){
    audioInitialized = false;

    SDL_CloseAudioDevice(dev);
}


void FillAudioDeviceBuffer(void* UserData, Uint8* DeviceBuffer, int Length)
{
    _audio->FillAudioBuffer(DeviceBuffer, 0, Length / 4);
}

void audioSetup(){
    //modifed from SDL docs: https://wiki.libsdl.org/SDL_OpenAudioDevice

    SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
    want.freq = SAMPLERATE;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.samples = 512;  // Lower buffer size for reduced audio latency (~23ms vs ~186ms)
    want.callback = FillAudioDeviceBuffer;
    

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        Logger_Write("Failed to open audio: %s", SDL_GetError());
    } else {
        if (have.format != want.format) { /* we let this one thing change. */
            Logger_Write("We didn't get requested audio format.");
        }
        SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
        audioInitialized = true;
    }
}

void _changeStretch(StretchOption newStretch){
    if (newStretch == PixelPerfect) {
        _screenWidth = PicoScreenWidth;
        _screenHeight = PicoScreenHeight;
    }
    else if (newStretch == StretchToFit) {
        int fitDim = std::min(_windowWidth, _windowHeight);
        _screenWidth = fitDim;
        _screenHeight = fitDim;
    }
    else if (newStretch == StretchToFill){
        _screenWidth = _windowWidth;
        _screenHeight = _windowHeight; 
    }
    else if (newStretch == PixelPerfectStretch) {
        int factor = std::min(_windowWidth / PicoScreenWidth, _windowHeight / PicoScreenHeight);
        if (factor < 1) factor = 1;
        _screenWidth = factor * PicoScreenWidth;
        _screenHeight = factor * PicoScreenHeight; 
    }
    else if (newStretch == FourByThreeVertPerfect) {
        _screenWidth = _maxNoStretchHeight * 4 / 3;
        _screenHeight = _maxNoStretchHeight; 
    }
    else if (newStretch == FourByThreeStretch) {
        _screenWidth = _windowHeight * 4 / 3;
        _screenHeight = _windowHeight; 
    }
    

    DestR.x = (_windowWidth - _screenWidth) / 2;
    DestR.y = (_windowHeight - _screenHeight) / 2;
    DestR.w = _screenWidth;
    DestR.h = _screenHeight;

    SrcR.x = 0;
    SrcR.y = 0;
    SrcR.w = PicoScreenWidth;
    SrcR.h = PicoScreenHeight;

    textureAngle = 0;
    flip = SDL_FLIP_NONE;
}

void Host::setPlatformParams(
    int windowWidth,
    int windowHeight,
    uint32_t sdlWindowFlags,
    uint32_t sdlRendererFlags,
    uint32_t sdlPixelFormat,
    std::string logFilePrefix,
    std::string customBiosLua,
    std::string cartDirectory) 
{
    _windowWidth = windowWidth;
    _windowHeight = windowHeight;
    
    //assume wide screen, height is limiting factor
    int maxNoStretchFactor = _windowHeight / PicoScreenHeight;

    _maxNoStretchWidth = maxNoStretchFactor * PicoScreenWidth;
    _maxNoStretchHeight = maxNoStretchFactor * PicoScreenHeight;

    _screenWidth = _maxNoStretchWidth;
    _screenHeight = _maxNoStretchHeight;

    _windowFlags = sdlWindowFlags;
    _rendererFlags = sdlRendererFlags;
    _pixelFormat = sdlPixelFormat;

    _logFilePrefix = logFilePrefix;
    _customBiosLua = customBiosLua;
    _cartDirectory = cartDirectory;

}


void Host::oneTimeSetup(Audio* audio){
#if defined(__PS4__) || defined(__ORBIS__)
    // Omit SDL_INIT_JOYSTICK on PS4: OpenOrbis SDL2's PS4_JoystickDetect calls
    // sceUserServiceGetLoginUserIdList + scePadOpen across 4 user slots on every
    // SDL_PollEvent call. PS4Host reads DualShock 4 directly via scePadReadState.
    uint32_t initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
#else
    uint32_t initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER | SDL_INIT_EVENTS;
#endif
    if (SDL_Init(initFlags) != 0)
    {
        Logger_Write("SDL could not initialize: %s\n", SDL_GetError());
        return;
    }

#if defined(__PS4__) || defined(__ORBIS__)
    window = SDL_CreateWindow("PicoPlay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _windowWidth, _windowHeight, _windowFlags);
#else
    window = SDL_CreateWindow("PicoPlay - 8-Bit Fantasy Arcade", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _windowWidth, _windowHeight, _windowFlags);
#endif
	if (!window) 
    { 
        Logger_Write("Error creating window: %s\n", SDL_GetError());
        quit = 1;
        return; 
    }
	
	renderer = SDL_CreateRenderer(window, -1, _rendererFlags);
    if (!renderer)
    {
        SDL_Surface* windowSurface = SDL_GetWindowSurface(window);
        if (windowSurface) {
            renderer = SDL_CreateSoftwareRenderer(windowSurface);
        }
    }
	if (!renderer) 
    { 
        Logger_Write("Error creating renderer: %s\n", SDL_GetError());
        quit = 1;
        return;
    }

    texture = SDL_CreateTexture(renderer, _pixelFormat, SDL_TEXTUREACCESS_STREAMING, PicoScreenWidth, PicoScreenHeight);
    if (!texture)
    {
        Logger_Write("Error creating texture: %s\n", SDL_GetError());
        quit = 1;
        return;
    }

    atexit(SDL_Quit);

    _audio = audio;
    audioSetup();

#if !defined(__PS4__) && !defined(__ORBIS__)
    joystickCount = SDL_NumJoysticks();
    for (int i = 0; i < joystickCount; i++) {
		if (SDL_JoystickOpen(i) == NULL) {
			Logger_Write("Failed to open joystick %d: %s\n", i, SDL_GetError());
		}
    }
#endif

    last_time = 0;
    now_time = 0;
    frame_time = 0;
    targetFrameTimeMs = 0;

    currKDown = 0;
    currKHeld = 0;

    loadSettingsIni();

    _changeStretch(stretch);

    scaleX = _screenWidth / (float)PicoScreenWidth;
    scaleY = _screenHeight / (float)PicoScreenHeight;
    mouseOffsetX = DestR.x;
    mouseOffsetY = DestR.y;
}

void Host::oneTimeCleanup(){
    saveSettingsIni();

    audioCleanup();

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Host::setTargetFps(int targetFps){
    targetFrameTimeMs = 1000 / targetFps;
}

void Host::changeStretch(){
    if (stretchKeyPressed && resizekey == YesResize) {
        StretchOption newStretch = stretch;

        if (stretch == PixelPerfectStretch) {
            newStretch = PixelPerfect;
        }
        else if (stretch == PixelPerfect) {
            newStretch = StretchToFit;
        }
        else if (stretch == StretchToFit) {
            newStretch = StretchToFill;
        }
        else if (stretch == StretchToFill) {
            newStretch = FourByThreeVertPerfect;
        }
        else if (stretch == FourByThreeVertPerfect) {
            newStretch = FourByThreeStretch;
        }
        else if (stretch == FourByThreeStretch) {
            newStretch = PixelPerfectStretch;
        }

        _changeStretch(newStretch);

        stretch = newStretch;
        scaleX = _screenWidth / (float)PicoScreenWidth;
        scaleY = _screenHeight / (float)PicoScreenHeight;
        mouseOffsetX = DestR.x;
        mouseOffsetY = DestR.y;
    }
}

void Host::forceStretch(StretchOption newStretch) {
	_changeStretch(newStretch);
	stretch = newStretch;
	scaleX = _screenWidth / (float)PicoScreenWidth;
	scaleY = _screenHeight / (float)PicoScreenHeight;
	mouseOffsetX = DestR.x;
	mouseOffsetY = DestR.y;
}

void Host::onWindowResized(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;

    _windowWidth = newWidth;
    _windowHeight = newHeight;

    int factor = std::min(_windowWidth / PicoScreenWidth, _windowHeight / PicoScreenHeight);
    if (factor < 1) factor = 1;

    _maxNoStretchWidth = factor * PicoScreenWidth;
    _maxNoStretchHeight = factor * PicoScreenHeight;

    _changeStretch(stretch);

    scaleX = _screenWidth / (float)PicoScreenWidth;
    scaleY = _screenHeight / (float)PicoScreenHeight;
    mouseOffsetX = DestR.x;
    mouseOffsetY = DestR.y;
}

bool Host::shouldQuit() {
    return quit == 1;
}

void Host::waitForTargetFps(){
#if defined(__PS4__) || defined(__ORBIS__)
    // On PS4, SDL_UpdateWindowSurface / PS4_UpdateWindowFramebuffer already blocks
    // on hardware 60Hz VSYNC via sceVideoOutSubmitFlip + WaitOnFlip (sceKernelWaitEqueue).
    // Any additional SDL_Delay here risks crossing the next VSYNC boundary and dropping to 30Hz.
    return;
#else
    now_time = SDL_GetTicks();
    if (last_time == 0) {
        last_time = now_time;
        return;
    }
    frame_time = now_time - last_time;

    // Smooth frame pacing: avoid sleeping tiny durations (< 3ms) that clash with VSYNC
    if (frame_time < targetFrameTimeMs) {
        uint32_t msToSleep = targetFrameTimeMs - frame_time;
        if (msToSleep >= 3) {
            // Wake up slightly before VSYNC deadline to avoid dropping frames on high refresh displays
            SDL_Delay(msToSleep - 1);
        }
    }
    last_time = SDL_GetTicks();
#endif
}


void Host::drawFrame(uint8_t* picoFb, uint8_t* screenPaletteMap, uint8_t drawMode){
#if defined(__PS4__) || defined(__ORBIS__)
    // Fast direct-to-window-surface scanline scaler for PS4:
    // Bypasses OpenOrbis SDL2's unoptimized (-O0) SW_RenderCopyEx / SDLgfx_rotateSurface
    // and avoids full-screen 1920x1080 SDL_RenderClear (8.3 MB) on every frame.
    SDL_Surface* winSurface = SDL_GetWindowSurface(window);
    if (winSurface && winSurface->pixels && DestR.w > 0 && DestR.h > 0) {
        static SDL_Rect prevDestR = {0, 0, 0, 0};
        static int clearFrames = 2;
        if (prevDestR.x != DestR.x || prevDestR.y != DestR.y ||
            prevDestR.w != DestR.w || prevDestR.h != DestR.h) {
            prevDestR = DestR;
            clearFrames = 2;
        }
        if (clearFrames > 0) {
            SDL_FillRect(winSurface, NULL, 0);
            clearFrames--;
        }

        // Pre-map the 16 active PICO-8 screen palette colors to the window surface pixel format
        uint32_t mappedPalette[16];
        for (int i = 0; i < 16; i++) {
            Color col = _paletteColors[screenPaletteMap[i] & 0x8f];
            mappedPalette[i] = SDL_MapRGB(winSurface->format, col.Red, col.Green, col.Blue);
        }

        // Unpack 128x128 4-bit framebuffer (8,192 bytes) in a single pass
        uint32_t raw128[PicoScreenWidth * PicoScreenHeight];
        for (int i = 0; i < (PicoScreenWidth * PicoScreenHeight) / 2; i++) {
            uint8_t byte = picoFb[i];
            raw128[i * 2]     = mappedPalette[byte & 0x0f];
            raw128[i * 2 + 1] = mappedPalette[byte >> 4];
        }

        // Apply PICO-8 drawMode (0x5f2c) on the compact 128x128 grid if active
        const uint32_t* src128 = raw128;
        uint32_t transformed128[PicoScreenWidth * PicoScreenHeight];
        if (drawMode != 0) {
            for (int y = 0; y < PicoScreenHeight; y++) {
                for (int x = 0; x < PicoScreenWidth; x++) {
                    int sx = x;
                    int sy = y;
                    switch (drawMode) {
                        case 1:   sx = x >> 1; break;
                        case 2:   sy = y >> 1; break;
                        case 3:   sx = x >> 1; sy = y >> 1; break;
                        case 4:   if (x >= 64) sx = 127 - x; break;
                        case 5:   if (y >= 64) sy = 127 - y; break;
                        case 6:
                        case 7:   if (x >= 64) sx = 127 - x; if (y >= 64) sy = 127 - y; break;
                        case 129: sx = 127 - x; break;
                        case 130: sy = 127 - y; break;
                        case 131: sx = 127 - x; sy = 127 - y; break;
                        case 133: sx = y; sy = 127 - x; break;
                        case 134: sx = 127 - x; sy = 127 - y; break;
                        case 135: sx = 127 - y; sy = x; break;
                        default:  break;
                    }
                    transformed128[y * PicoScreenWidth + x] = raw128[sy * PicoScreenWidth + sx];
                }
            }
            src128 = transformed128;
        }

        // Precompute horizontal source X lookup table
        int srcXTable[2048];
        int dstW = std::min(DestR.w, std::min(2048, winSurface->w - DestR.x));
        int dstH = std::min(DestR.h, winSurface->h - DestR.y);
        for (int dx = 0; dx < dstW; dx++) {
            srcXTable[dx] = (dx * PicoScreenWidth) / DestR.w;
        }

        // Scale 128 rows to DestR.h: only 128 unique scanlines are computed;
        // duplicate rows are memcpy'd directly from the L1-cached previous scanline.
        int prevSy = -1;
        uint32_t* prevDstRow = nullptr;
        uint8_t* dstBase = (uint8_t*)winSurface->pixels + DestR.y * winSurface->pitch + DestR.x * sizeof(uint32_t);
        size_t rowBytes = (size_t)dstW * sizeof(uint32_t);

        for (int dy = 0; dy < dstH; dy++) {
            int sy = (dy * PicoScreenHeight) / DestR.h;
            uint32_t* dstRow = (uint32_t*)(dstBase + dy * winSurface->pitch);
            if (sy == prevSy && prevDstRow != nullptr) {
                memcpy(dstRow, prevDstRow, rowBytes);
            } else {
                const uint32_t* srcRow = &src128[sy * PicoScreenWidth];
                for (int dx = 0; dx < dstW; dx++) {
                    dstRow[dx] = srcRow[srcXTable[dx]];
                }
                prevSy = sy;
                prevDstRow = dstRow;
            }
        }

        SDL_UpdateWindowSurface(window);
        return;
    }
#endif

    //clear screen to all black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    SDL_LockTexture(texture, NULL, &pixels, &pitch);

    for (int y = 0; y < PicoScreenHeight; y ++){
        for (int x = 0; x < PicoScreenWidth; x ++){
            uint8_t c = getPixelNibble(x, y, picoFb);
            // Apply 0x8F mask per PICO-8 spec: system color indices are masked with 0x8F
            // This maps values like 149 to valid palette indices (149 & 0x8F = 133)
            Color col = _paletteColors[screenPaletteMap[c] & 0x8f];

            base = ((Uint8 *)pixels) + (4 * ( y * PicoScreenHeight + x));
            base[0] = col.Blue;
            base[1] = col.Green;
            base[2] = col.Red;
            base[3] = col.Alpha;
        }
    }

    SrcR.x = 0;
    SrcR.y = 0;

    switch(drawMode){
        case 1:
            SrcR.w = 64;
            SrcR.h = PicoScreenHeight;
            textureAngle = 0;
            flip = SDL_FLIP_NONE;
            break;
        case 2:
            SrcR.w = PicoScreenWidth;
            SrcR.h = 64;
            textureAngle = 0;
            flip = SDL_FLIP_NONE;
            break;
        case 3:
            SrcR.w = 64;
            SrcR.h = 64;
            textureAngle = 0;
            flip = SDL_FLIP_NONE;
            break;
        //todo: mirroring
        //case 4,6,7
        case 129:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 0;
            flip = SDL_FLIP_HORIZONTAL;
            break;
        case 130:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 0;
            flip = SDL_FLIP_VERTICAL;
            break;
        case 131:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 0;
            flip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
            break;
        case 133:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 90;
            flip = SDL_FLIP_NONE;
            break;
        case 134:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 180;
            flip = SDL_FLIP_NONE;
            break;
        case 135:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 270;
            flip = SDL_FLIP_NONE;
            break;
        default:
            SrcR.w = PicoScreenWidth;
            SrcR.h = PicoScreenHeight;
            textureAngle = 0;
            flip = SDL_FLIP_NONE;
            break;
    }
    

    postFlipFunction();
}

bool Host::shouldFillAudioBuff(){
    return false;
}

void* Host::getAudioBufferPointer(){
    return nullptr;
}

size_t Host::getAudioBufferSize(){
    return 0;
}

void Host::playFilledAudioBuffer(){
}

bool Host::shouldRunMainLoop(){
    if (shouldQuit()){
        return false;
    }

    return true;
}

const char* Host::logFilePrefix() {
    return _logFilePrefix.c_str();
}

std::string Host::customBiosLua() {
    return _customBiosLua;
}

std::string Host::getCartDirectory() {
    return _cartDirectory;
}

