#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <switch.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iterator>
#include <dirent.h>
#include <cstdio>
#include <filesystem>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include "led.hpp"
#include "lang.hpp"
#include "spl.hpp"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "spl.hpp"

#define IRAM_PAYLOAD_MAX_SIZE 0x24000
static u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE];
char Logs[2024];
bool isXSOS;

extern "C" {
#include "reboot.h"
}
/// HidControllerKeys
typedef enum {
    KEY_A            = BIT(0),       ///< A
    KEY_B            = BIT(1),       ///< B
    KEY_X            = BIT(2),       ///< X
    KEY_Y            = BIT(3),       ///< Y
    KEY_LSTICK       = BIT(4),       ///< Left Stick Button
    KEY_RSTICK       = BIT(5),       ///< Right Stick Button
    KEY_L            = BIT(6),       ///< L
    KEY_R            = BIT(7),       ///< R
    KEY_ZL           = BIT(8),       ///< ZL
    KEY_ZR           = BIT(9),       ///< ZR
    KEY_PLUS         = BIT(10),      ///< Plus
    KEY_MINUS        = BIT(11),      ///< Minus
    KEY_DLEFT        = BIT(12),      ///< D-Pad Left
    KEY_DUP          = BIT(13),      ///< D-Pad Up
    KEY_DRIGHT       = BIT(14),      ///< D-Pad Right
    KEY_DDOWN        = BIT(15),      ///< D-Pad Down
    KEY_LSTICK_LEFT  = BIT(16),      ///< Left Stick Left
    KEY_LSTICK_UP    = BIT(17),      ///< Left Stick Up
    KEY_LSTICK_RIGHT = BIT(18),      ///< Left Stick Right
    KEY_LSTICK_DOWN  = BIT(19),      ///< Left Stick Down
    KEY_RSTICK_LEFT  = BIT(20),      ///< Right Stick Left
    KEY_RSTICK_UP    = BIT(21),      ///< Right Stick Up
    KEY_RSTICK_RIGHT = BIT(22),      ///< Right Stick Right
    KEY_RSTICK_DOWN  = BIT(23),      ///< Right Stick Down
    KEY_SL_LEFT      = BIT(24),      ///< SL on Left Joy-Con
    KEY_SR_LEFT      = BIT(25),      ///< SR on Left Joy-Con
    KEY_SL_RIGHT     = BIT(26),      ///< SL on Right Joy-Con
    KEY_SR_RIGHT     = BIT(27),      ///< SR on Right Joy-Con

    KEY_HOME         = BIT(18),      ///< HOME button, only available for use with HiddbgHdlsState::buttons.
    KEY_CAPTURE      = BIT(19),      ///< Capture button, only available for use with HiddbgHdlsState::buttons.

    // Pseudo-key for at least one finger on the touch screen
    KEY_TOUCH       = BIT(28),

    // Buttons by orientation (for single Joy-Con), also works with Joy-Con pairs, Pro Controller
    KEY_JOYCON_RIGHT = BIT(0),
    KEY_JOYCON_DOWN  = BIT(1),
    KEY_JOYCON_UP    = BIT(2),
    KEY_JOYCON_LEFT  = BIT(3),

    // Generic catch-all directions, also works for single Joy-Con
    KEY_UP    = KEY_DUP     | KEY_LSTICK_UP    | KEY_RSTICK_UP,    ///< D-Pad Up or Sticks Up
    KEY_DOWN  = KEY_DDOWN   | KEY_LSTICK_DOWN  | KEY_RSTICK_DOWN,  ///< D-Pad Down or Sticks Down
    KEY_LEFT  = KEY_DLEFT   | KEY_LSTICK_LEFT  | KEY_RSTICK_LEFT,  ///< D-Pad Left or Sticks Left
    KEY_RIGHT = KEY_DRIGHT  | KEY_LSTICK_RIGHT | KEY_RSTICK_RIGHT, ///< D-Pad Right or Sticks Right
    KEY_SL    = KEY_SL_LEFT | KEY_SL_RIGHT,                        ///< SL on Left or Right Joy-Con
    KEY_SR    = KEY_SR_LEFT | KEY_SR_RIGHT,                        ///< SR on Left or Right Joy-Con
} HidControllerKeys;

using namespace std;
Result Init_Services(void)
{
	Result ret = 0;
	if (R_FAILED(ret = splInitialize())) {return 1;}
	if (R_FAILED(ret = hiddbgInitialize())) {return 1;}
	if (R_FAILED(ret = psmInitialize())) {return 1;}
	return ret;
}

void close_Services()
{
	setsysExit();
	splExit();
	smExit();
	hiddbgExit();
	psmExit();
}

void copy_me(string origen, string destino) {
	ifstream source(origen, ios::binary);
	ofstream dest(destino, ios::binary);
	dest << source.rdbuf();
	source.close();
	dest.close();
}

bool is_patched = false;
void CheckHardware()
{
	/*Check if is Ipatched/Mariko */
	if (spl::GetHardwareType() == "Iowa" || spl::GetHardwareType() == "Hoag" || spl::GetHardwareType() == "Calcio" || spl::HasRCMbugPatched())
	{
		is_patched = true;
	}
}

void SetupClean (){
	copy_me("romfs:/TegraExplorer.bin", "/Haku33.bin");
	copy_me("romfs:/startup.te", "/startup.te");
	if (is_patched){
		led_on(1);
		mkdir("sdmc:/bootloader",0777);
		mkdir("sdmc:/bootloader/res",0777);
		mkdir("sdmc:/bootloader/ini",0777);
		copy_me("romfs:/ini/Haku33.bmp", "/bootloader/res/Haku33.bmp");
		copy_me("romfs:/ini/Haku33.ini", "/bootloader/ini/Haku33.ini");
		copy_me("romfs:/hekate_keys.ini", "/bootloader/hekate_keys.ini");
		spsmInitialize();
		spsmShutdown(true);
	}else{
		led_on(1);
		bpcInitialize();
		if(init_slp()){reboot_to_payload();}
		bpcExit();
	}
}

int main(int argc, char **argv)
{
	set_LANG();
	romfsInit();
	consoleInit(NULL);
	Init_Services();
	CheckHardware();
	PadState pad;
	padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    Result rc = 0;

	//keys
	while (appletMainLoop())
	{
	    //hidScanInput();
		padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);

		
		u32 minus = 0;
		u32 more = 0;
		u32 LT = 0;
		u32 RT = 0;
	
		if (kHeld & KEY_ZL)
			LT = 4;
		if (kHeld & KEY_ZR)
			RT = 4;
		if (kHeld & KEY_MINUS)
			minus = 4;
		if (kHeld & KEY_PLUS)
			more = 4;

		if (kHeld & KEY_ZL && kHeld & KEY_ZR && kHeld & KEY_MINUS && kHeld & KEY_PLUS)
		{
			minus = 2;
			more = 2;
			LT = 2;
			RT = 2;
		}

		//main menu
		consoleClear();
			printf("\x1b[32;1m*\x1b[0m %s v%s Kronos2308, Hard Reset \n\n",TITLE, VERSION);
			printf(LG.text1);
			printf(LG.text2);
			printf(LG.text3);
			printf(LG.text4);
			if (is_patched){
				printf("\n\n %s",LG.text7);
			} 
			printf("\n\x1b[31;1m%s \x1b[0m ",Logs);
			printf(LG.text5);


		consoleUpdate(NULL);
		
		//call clean after combo
		if (kHeld & KEY_A)
		{
			SetupClean();
			//break;
		}
		
		//exit
		if (kDown & KEY_B || kDown & KEY_Y || kDown & KEY_X)
		{
			break;
		}
	}

	//cansel
	close_Services();
	consoleExit(NULL);
	return 0;
}
