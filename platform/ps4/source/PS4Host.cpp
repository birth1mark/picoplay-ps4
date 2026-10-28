#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

#include "../../SDL2Common/source/sdl2basehost.h"
#include "../../../source/host.h"
#include "../../../source/hostVmShared.h"
#include "../../../source/nibblehelpers.h"
#include "../../../source/logger.h"
#include "../../../source/filehelpers.h"

// SDL2 includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>

#define WINDOW_SIZE_X 1920
#define WINDOW_SIZE_Y 1080

// OpenOrbis SDL2 requires the software renderer (PS4_CreateRenderer is a stub)
#define WINDOW_FLAGS 0
#define RENDERER_FLAGS SDL_RENDERER_SOFTWARE
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

// OpenOrbis SDL_sysjoystick.c DualShock 4 button indices (SDL_JOYBUTTONDOWN / SDL_JOYBUTTONUP)
enum OrbisSdlJoyButton {
    PS4_JOY_BTN_CROSS     = 0,
    PS4_JOY_BTN_CIRCLE    = 1,
    PS4_JOY_BTN_SQUARE    = 2,
    PS4_JOY_BTN_TRIANGLE  = 3,
    PS4_JOY_BTN_L1        = 4,
    PS4_JOY_BTN_R1        = 5,
    PS4_JOY_BTN_OPTIONS   = 9,
    PS4_JOY_BTN_L3        = 11,
    PS4_JOY_BTN_R3        = 12,
    PS4_JOY_BTN_DPAD_UP   = 13,
    PS4_JOY_BTN_DPAD_DOWN = 14,
    PS4_JOY_BTN_DPAD_LEFT = 15,
    PS4_JOY_BTN_DPAD_RIGHT= 16,
    PS4_JOY_BTN_TOUCHPAD  = 17,
    PS4_JOY_BTN_L2        = 18,
    PS4_JOY_BTN_R2        = 19
};

// DualShock 4 button mappings for SDL_CONTROLLER_BUTTON
#define DS4_BTN_CROSS       SDL_CONTROLLER_BUTTON_A
#define DS4_BTN_CIRCLE      SDL_CONTROLLER_BUTTON_B
#define DS4_BTN_SQUARE      SDL_CONTROLLER_BUTTON_X
#define DS4_BTN_TRIANGLE    SDL_CONTROLLER_BUTTON_Y
#define DS4_BTN_OPTIONS     SDL_CONTROLLER_BUTTON_START
#define DS4_BTN_SHARE       SDL_CONTROLLER_BUTTON_BACK
#define DS4_BTN_L1          SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define DS4_BTN_R1          SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define DS4_BTN_DPAD_UP     SDL_CONTROLLER_BUTTON_DPAD_UP
#define DS4_BTN_DPAD_DOWN   SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define DS4_BTN_DPAD_LEFT   SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define DS4_BTN_DPAD_RIGHT  SDL_CONTROLLER_BUTTON_DPAD_RIGHT

// Analog joystick dead zone (0..255 centered at 128 -> deadzone 32 around 128)
const int PAD_STICK_DEAD_ZONE = 32;
static int jxDir = 0;
static int jyDir = 0;
static int ps4PadHandle = -1;
static uint32_t prevPadButtons = 0;

static SDL_Event event;
static SDL_Point touchLocation = { 128 / 2, 128 / 2 };

static string _ps4SettingsDir = "/data/pico8-ps4";
static string _ps4SettingsPrefix = "/data/pico8-ps4/";
static string _ps4CartDir = "/data/pico8-ps4/carts";
static string _ps4CustomBiosLua = 
    "cartpath = \"/data/pico8-ps4/carts/\"\n"
    "selectbtn = \"x\"\n"
    "pausebtn = \"options\"";

static void ensure_dir(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0777);
    }
}

Host::Host(int windowWidth, int windowHeight) {
    // Ensure PICO-8 PS4 internal storage paths exist on PS4 /data partition
    ensure_dir("/data");
    ensure_dir("/data/pico8-ps4");
    ensure_dir("/data/pico8-ps4/cdata");
    ensure_dir("/data/pico8-ps4/carts");

    // Check if USB storage with pico8 carts is plugged in
    struct stat st;
    if (stat("/mnt/usb0/pico8", &st) == 0 && S_ISDIR(st.st_mode)) {
        _ps4CartDir = "/mnt/usb0/pico8";
    } else if (stat("/mnt/usb1/pico8", &st) == 0 && S_ISDIR(st.st_mode)) {
        _ps4CartDir = "/mnt/usb1/pico8";
    }

    setPlatformParams(
        WINDOW_SIZE_X,
        WINDOW_SIZE_Y,
        WINDOW_FLAGS,
        RENDERER_FLAGS,
        PIXEL_FORMAT,
        _ps4SettingsPrefix,
        _ps4CustomBiosLua,
        _ps4CartDir
    );
}

InputState_t Host::scanInput() {
    currKDown = 0;
    uint8_t kUp = 0;
    int prevJxDir = jxDir;
    int prevJyDir = jyDir;
    stretchKeyPressed = false;
    uint8_t mouseBtnState = 0;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_JOYBUTTONDOWN:
                switch (event.jbutton.button) {
                    case PS4_JOY_BTN_OPTIONS:    currKDown |= P8_KEY_PAUSE; break;
                    case PS4_JOY_BTN_DPAD_LEFT:  currKDown |= P8_KEY_LEFT;  break;
                    case PS4_JOY_BTN_DPAD_RIGHT: currKDown |= P8_KEY_RIGHT; break;
                    case PS4_JOY_BTN_DPAD_UP:    currKDown |= P8_KEY_UP;    break;
                    case PS4_JOY_BTN_DPAD_DOWN:  currKDown |= P8_KEY_DOWN;  break;
                    case PS4_JOY_BTN_CROSS:      currKDown |= P8_KEY_X;     break;
                    case PS4_JOY_BTN_CIRCLE:     currKDown |= P8_KEY_O;     break;
                    case PS4_JOY_BTN_SQUARE:     currKDown |= P8_KEY_O;     break;
                    case PS4_JOY_BTN_TRIANGLE:   stretchKeyPressed = true;  break;
                    case PS4_JOY_BTN_L1:         lDown = true;              break;
                    case PS4_JOY_BTN_R1:         rDown = true;              break;
                }
                break;

            case SDL_JOYBUTTONUP:
                switch (event.jbutton.button) {
                    case PS4_JOY_BTN_OPTIONS:    kUp |= P8_KEY_PAUSE; break;
                    case PS4_JOY_BTN_DPAD_LEFT:  kUp |= P8_KEY_LEFT;  break;
                    case PS4_JOY_BTN_DPAD_RIGHT: kUp |= P8_KEY_RIGHT; break;
                    case PS4_JOY_BTN_DPAD_UP:    kUp |= P8_KEY_UP;    break;
                    case PS4_JOY_BTN_DPAD_DOWN:  kUp |= P8_KEY_DOWN;  break;
                    case PS4_JOY_BTN_CROSS:      kUp |= P8_KEY_X;     break;
                    case PS4_JOY_BTN_CIRCLE:     kUp |= P8_KEY_O;     break;
                    case PS4_JOY_BTN_SQUARE:     kUp |= P8_KEY_O;     break;
                    case PS4_JOY_BTN_L1:         lDown = false;       break;
                    case PS4_JOY_BTN_R1:         rDown = false;       break;
                }
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                switch (event.cbutton.button) {
                    case DS4_BTN_OPTIONS:    currKDown |= P8_KEY_PAUSE; break;
                    case DS4_BTN_DPAD_LEFT:  currKDown |= P8_KEY_LEFT;  break;
                    case DS4_BTN_DPAD_RIGHT: currKDown |= P8_KEY_RIGHT; break;
                    case DS4_BTN_DPAD_UP:    currKDown |= P8_KEY_UP;    break;
                    case DS4_BTN_DPAD_DOWN:  currKDown |= P8_KEY_DOWN;  break;
                    case DS4_BTN_CROSS:      currKDown |= P8_KEY_X;     break;
                    case DS4_BTN_CIRCLE:     currKDown |= P8_KEY_O;     break;
                    case DS4_BTN_SQUARE:     currKDown |= P8_KEY_O;     break;
                    case DS4_BTN_TRIANGLE:   stretchKeyPressed = true;  break;
                    case DS4_BTN_L1:         lDown = true;              break;
                    case DS4_BTN_R1:         rDown = true;              break;
                }
                break;

            case SDL_CONTROLLERBUTTONUP:
                switch (event.cbutton.button) {
                    case DS4_BTN_OPTIONS:    kUp |= P8_KEY_PAUSE; break;
                    case DS4_BTN_DPAD_LEFT:  kUp |= P8_KEY_LEFT;  break;
                    case DS4_BTN_DPAD_RIGHT: kUp |= P8_KEY_RIGHT; break;
                    case DS4_BTN_DPAD_UP:    kUp |= P8_KEY_UP;    break;
                    case DS4_BTN_DPAD_DOWN:  kUp |= P8_KEY_DOWN;  break;
                    case DS4_BTN_CROSS:      kUp |= P8_KEY_X;     break;
                    case DS4_BTN_CIRCLE:     kUp |= P8_KEY_O;     break;
                    case DS4_BTN_SQUARE:     kUp |= P8_KEY_O;     break;
                    case DS4_BTN_L1:         lDown = false;       break;
                    case DS4_BTN_R1:         rDown = false;       break;
                }
                break;

            case SDL_QUIT:
                quit = 1;
                break;
        }
    }

    // Read DualShock 4 state directly via scePadReadState (guarantees analog stick & buttons work on PS4)
    if (ps4PadHandle < 0) {
        OrbisUserServiceUserId userId = ORBIS_USER_SERVICE_USER_ID_INVALID;
        if (sceUserServiceGetInitialUser(&userId) == 0 && userId != ORBIS_USER_SERVICE_USER_ID_INVALID) {
            ps4PadHandle = scePadGetHandle(userId, 0, 0);
            if (ps4PadHandle < 0) {
                ps4PadHandle = scePadOpen(userId, 0, 0, NULL);
            }
        }
    }
    if (ps4PadHandle >= 0) {
        OrbisPadData padData;
        memset(&padData, 0, sizeof(padData));
        if (scePadReadState(ps4PadHandle, &padData) == 0 && padData.connected) {
            int lx = (int)padData.leftStick.x - 128;
            int ly = (int)padData.leftStick.y - 128;
            jxDir = (lx < -PAD_STICK_DEAD_ZONE) ? -1 : ((lx > PAD_STICK_DEAD_ZONE) ? 1 : 0);
            jyDir = (ly < -PAD_STICK_DEAD_ZONE) ? -1 : ((ly > PAD_STICK_DEAD_ZONE) ? 1 : 0);

            uint32_t btns = padData.buttons;
            uint32_t justPressed = btns & ~prevPadButtons;
            uint32_t justReleased = ~btns & prevPadButtons;
            prevPadButtons = btns;

            if (justPressed & ORBIS_PAD_BUTTON_OPTIONS)  currKDown |= P8_KEY_PAUSE;
            if (justPressed & ORBIS_PAD_BUTTON_LEFT)     currKDown |= P8_KEY_LEFT;
            if (justPressed & ORBIS_PAD_BUTTON_RIGHT)    currKDown |= P8_KEY_RIGHT;
            if (justPressed & ORBIS_PAD_BUTTON_UP)       currKDown |= P8_KEY_UP;
            if (justPressed & ORBIS_PAD_BUTTON_DOWN)     currKDown |= P8_KEY_DOWN;
            if (justPressed & ORBIS_PAD_BUTTON_CROSS)    currKDown |= P8_KEY_X;
            if (justPressed & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_SQUARE)) currKDown |= P8_KEY_O;
            if (justPressed & ORBIS_PAD_BUTTON_TRIANGLE) stretchKeyPressed = true;

            if (justReleased & ORBIS_PAD_BUTTON_OPTIONS) kUp |= P8_KEY_PAUSE;
            if (justReleased & ORBIS_PAD_BUTTON_LEFT)    kUp |= P8_KEY_LEFT;
            if (justReleased & ORBIS_PAD_BUTTON_RIGHT)   kUp |= P8_KEY_RIGHT;
            if (justReleased & ORBIS_PAD_BUTTON_UP)      kUp |= P8_KEY_UP;
            if (justReleased & ORBIS_PAD_BUTTON_DOWN)    kUp |= P8_KEY_DOWN;
            if (justReleased & ORBIS_PAD_BUTTON_CROSS)   kUp |= P8_KEY_X;
            if (justReleased & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_SQUARE)) kUp |= P8_KEY_O;

            lDown = (btns & ORBIS_PAD_BUTTON_L1) != 0;
            rDown = (btns & ORBIS_PAD_BUTTON_R1) != 0;
        }
    }

    // L1 + R1 + Share / Options combo to exit back to PS4 menu
    if (lDown && rDown && (currKDown & P8_KEY_PAUSE)) {
        quit = 1;
    }

    currKHeld |= currKDown;
    currKHeld &= ~kUp;

    // Analog stick mapped to D-pad inputs
    if (jxDir > 0) {
        currKHeld |= P8_KEY_RIGHT;
        currKHeld &= ~(P8_KEY_LEFT);
        if (prevJxDir != jxDir) currKDown |= P8_KEY_RIGHT;
    } else if (jxDir < 0) {
        currKHeld |= P8_KEY_LEFT;
        currKHeld &= ~(P8_KEY_RIGHT);
        if (prevJxDir != jxDir) currKDown |= P8_KEY_LEFT;
    } else if (prevJxDir != 0) {
        currKHeld &= ~(P8_KEY_RIGHT | P8_KEY_LEFT);
        currKDown &= ~(P8_KEY_RIGHT | P8_KEY_LEFT);
    }

    if (jyDir > 0) {
        currKHeld |= P8_KEY_DOWN;
        currKHeld &= ~(P8_KEY_UP);
        if (prevJyDir != jyDir) currKDown |= P8_KEY_DOWN;
    } else if (jyDir < 0) {
        currKHeld |= P8_KEY_UP;
        currKHeld &= ~(P8_KEY_DOWN);
        if (prevJyDir != jyDir) currKDown |= P8_KEY_UP;
    } else if (prevJyDir != 0) {
        currKHeld &= ~(P8_KEY_UP | P8_KEY_DOWN);
        currKDown &= ~(P8_KEY_UP | P8_KEY_DOWN);
    }

    return InputState_t {
        currKDown,
        currKHeld,
        (int16_t)touchLocation.x,
        (int16_t)touchLocation.y,
        mouseBtnState
    };
}

vector<string> Host::listcarts() {
    vector<string> carts;
    DIR *dir;
    struct dirent *ent;

    // Scan primary cart directory (/data/pico8-ps4/carts or USB)
    if ((dir = opendir(_cartDirectory.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            if (isCartFile(ent->d_name)) {
                carts.push_back(_cartDirectory + "/" + ent->d_name);
            }
        }
        closedir(dir);
    }

    // Also scan bundled application carts (/app0/carts) and legacy (/data/orbis8/carts)
    const char* extraCartDirs[] = { "/app0/carts", "/data/orbis8/carts" };
    for (const char* extraDir : extraCartDirs) {
        if (_cartDirectory != extraDir && (dir = opendir(extraDir)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                if (isCartFile(ent->d_name)) {
                    carts.push_back(string(extraDir) + "/" + ent->d_name);
                }
            }
            closedir(dir);
        }
    }

    // Also scan USB storage if not already scanned
    const char* usbPaths[] = { "/mnt/usb0/pico8", "/mnt/usb1/pico8" };
    for (const char* usbPath : usbPaths) {
        if (_cartDirectory != usbPath && (dir = opendir(usbPath)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                if (isCartFile(ent->d_name)) {
                    carts.push_back(string(usbPath) + "/" + ent->d_name);
                }
            }
            closedir(dir);
        }
    }

    return carts;
}

std::vector<std::string> Host::listdirs() {
    std::vector<std::string> dirs;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(_cartDirectory.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            if (ent->d_type == DT_DIR) {
                dirs.push_back(ent->d_name);
            }
        }
        closedir(dir);
    }

    return dirs;
}
