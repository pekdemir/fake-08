
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
using namespace std;

#include "../../SDL2Common/source/sdl2basehost.h"

#include "../../../source/host.h"
#include "../../../source/hostVmShared.h"
#include "../../../source/nibblehelpers.h"
#include "../../../source/logger.h"
#include "../../../source/filehelpers.h"

// sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

#define WINDOW_SIZE_X 1280
#define WINDOW_SIZE_Y 720

// #define BUTTON_L2 16
// #define BUTTON_R2 17

#define WINDOW_FLAGS SDL_WINDOW_SHOWN

#define RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888


//Analog joystick dead zone
const int JOYSTICK_DEAD_ZONE = 8000;
int jxDir = 0;
int jyDir = 0;
SDL_Event event;

string _desktopSdl2SettingsDir = "/mnt/SDCARD/fake08";
string _desktopSdl2SettingsPrefix = "/mnt/SDCARD/fake08/";
string _trimuiCartDir = "/mnt/SDCARD/Roms/PICO8";
string _desktopSdl2customBiosLua = "cartpath = \"/mnt/SDCARD/Roms/PICO8/\"\n";


Host::Host(int windowWidth, int windowHeight)  {
    SDL_DisplayMode current;

    SDL_Init(SDL_INIT_VIDEO);

    int should_be_zero = SDL_GetCurrentDisplayMode(0, &current);

    if(should_be_zero != 0) {
      // In case of error...
      SDL_Log("Could not get display mode for video display #%d: %s", 0, SDL_GetError());
    }

    else {
      // On success, print the current display mode.
      SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.", 0, current.w, current.h, current.refresh_rate);
    }

    // int WINDOW_SIZE_X= windowWidth == 0 ? current.w : windowWidth;
    // int WINDOW_SIZE_Y= windowHeight == 0 ? current.h : windowHeight;

    struct stat st = {0};

    int res = chdir(getenv("HOME"));
    if (res == 0 && stat(_desktopSdl2SettingsDir.c_str(), &st) == -1) {
        res = mkdir(_desktopSdl2SettingsDir.c_str(), 0777);
    }
    
    string cartdatadir = _desktopSdl2SettingsPrefix + "cdata";
    if (res == 0 && stat(cartdatadir.c_str(), &st) == -1) {
        res = mkdir(cartdatadir.c_str(), 0777);
    }

    setPlatformParams(
        WINDOW_SIZE_X,
        WINDOW_SIZE_Y,
        WINDOW_FLAGS,
        RENDERER_FLAGS,
        PIXEL_FORMAT,
        _desktopSdl2SettingsPrefix,
        _desktopSdl2customBiosLua,
        _trimuiCartDir
    );
}

InputState_t Host::scanInput(){ 
    currKDown = 0;
    uint8_t kUp = 0;
    int prevJxDir = jxDir;
    int prevJyDir = jyDir;
    stretchKeyPressed = false;
    
    uint8_t mouseBtnState = 0;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_CONTROLLERBUTTONDOWN :
                switch (event.cbutton.button)
                {
                    case SDL_CONTROLLER_BUTTON_START:         currKDown |= P8_KEY_PAUSE; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     currKDown |= P8_KEY_LEFT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    currKDown |= P8_KEY_RIGHT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:       currKDown |= P8_KEY_UP; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     currKDown |= P8_KEY_DOWN; break;
                    case SDL_CONTROLLER_BUTTON_B:             currKDown |= P8_KEY_X; break;
                    case SDL_CONTROLLER_BUTTON_A:             currKDown |= P8_KEY_O; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  lDown = true; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: rDown = true; break;
                    case SDL_CONTROLLER_BUTTON_BACK:          stretchKeyPressed = true; break;
                    // case SDL_CONTROLLER_BUTTON_GUIDE:         quit = 1; break;
                }
                break;

            case SDL_CONTROLLERBUTTONUP :
                switch (event.cbutton.button)
                {
                    case SDL_CONTROLLER_BUTTON_START:         kUp |= P8_KEY_PAUSE; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     kUp |= P8_KEY_LEFT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    kUp |= P8_KEY_RIGHT; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:       kUp |= P8_KEY_UP; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     kUp |= P8_KEY_DOWN; break;
                    case SDL_CONTROLLER_BUTTON_B:             kUp |= P8_KEY_X; break;
                    case SDL_CONTROLLER_BUTTON_A:             kUp |= P8_KEY_O; break;
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  lDown = false; break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: rDown = false; break;
                }
               break;

            case SDL_CONTROLLERAXISMOTION :
                if (event.caxis.which == 0)
                {
                    //X axis motion
                    if( event.caxis.axis == 0 )
                    {
                        //Left of dead zone
                        if( event.caxis.value < -JOYSTICK_DEAD_ZONE )
                        {
                            jxDir = -1;
                        }
                        //Right of dead zone
                        else if( event.caxis.value > JOYSTICK_DEAD_ZONE )
                        {
                            jxDir =  1;
                        }
                        else
                        {
                            jxDir = 0;
                        }
                    }
                    //Y axis motion
                    else if( event.caxis.axis == 1 )
                    {
                        //Below of dead zone
                        if( event.caxis.value < -JOYSTICK_DEAD_ZONE )
                        {
                            jyDir = -1;
                        }
                        //Above of dead zone
                        else if( event.caxis.value > JOYSTICK_DEAD_ZONE )
                        {
                            jyDir =  1;
                        }
                        else
                        {
                            jyDir = 0;
                        }
                    }
                }
               break;

            case SDL_QUIT:
                quit = 1;
                break;
        }
    }

    if (lDown && rDown){
        quit = 1;
    }

    currKHeld |= currKDown;
    currKHeld ^= kUp;


    //Convert joystick direction to kHeld and kDown values
    if (jxDir > 0) {
        currKHeld |= P8_KEY_RIGHT;
        currKHeld &= ~(P8_KEY_LEFT);

        if (prevJxDir != jxDir){
            currKDown |= P8_KEY_RIGHT;
        }
    }
    else if (jxDir < 0) {
        currKHeld |= P8_KEY_LEFT;
        currKHeld &= ~(P8_KEY_RIGHT);

        if (prevJxDir != jxDir){
            currKDown |= P8_KEY_LEFT;
        }
    }
    else if (prevJxDir != 0){
        currKHeld &= ~(P8_KEY_RIGHT);
        currKHeld &= ~(P8_KEY_LEFT);
        currKDown &= ~(P8_KEY_RIGHT);
        currKDown &= ~(P8_KEY_LEFT);
    }
    
    if (jyDir > 0) {
        currKHeld |= P8_KEY_DOWN;
        currKHeld &= ~(P8_KEY_UP);

        if (prevJyDir != jyDir){
            currKDown |= P8_KEY_DOWN;
        }
    }
    else if (jyDir < 0) {
        currKHeld |= P8_KEY_UP;
        currKHeld &= ~(P8_KEY_DOWN);

        if (prevJyDir != jyDir){
            currKDown |= P8_KEY_UP;
        }
    }
    else if (prevJyDir != 0){
        currKHeld &= ~(P8_KEY_UP);
        currKHeld &= ~(P8_KEY_DOWN);
        currKDown &= ~(P8_KEY_UP);
        currKDown &= ~(P8_KEY_DOWN);
    }
    


    return InputState_t {
        currKDown,
        currKHeld,
        0,
        0,
        mouseBtnState
    };
    
}


vector<string> Host::listcarts(){
    vector<string> carts;

    Logger_Write("Host::listcarts scanning directory: %s\n", _cartDirectory.c_str());

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (_cartDirectory.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (isCartFile(ent->d_name)){
                carts.push_back(ent->d_name);
            }
        }
        closedir (dir);
        Logger_Write("Host::listcarts found %zu carts\n", carts.size());
    } else {
        /* could not open directory */
        Logger_Write("Host::listcarts failed to open directory: %s\n", _cartDirectory.c_str());
        perror ("");
    }
    
    return carts;
}

std::vector<std::string> Host::listdirs() {
    std::vector<std::string> dirs;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (_cartDirectory.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if (ent->d_name[0] == '.') {
                continue;
            }
            std::string fullPath = _cartDirectory + "/" + ent->d_name;
            DIR* testDir = opendir(fullPath.c_str());
            if (testDir != NULL) {
                closedir(testDir);
                dirs.push_back(ent->d_name);
            }
        }
        closedir (dir);
    }
    
    return dirs;
}


