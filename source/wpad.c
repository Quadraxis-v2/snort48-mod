/*
	SNORT48+ - wpad.c
	Manages Wiimote functions for Snort48+.
*/

#include <grrlib.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>

#include "constants.h"
#include "video.h"

typedef struct{
	u16 x;
	u16 y;
	u16 width;
	u16 height;
} hotSpot;
hotSpot hotSpots[MAXHOTSPOTS];

//Variables
ir_t P1Mote;

int P1MX, P1MY, P1MA;
u32 WPADKeyDown;
u32 WPADKeyHeld;

void Wpad_AddHotSpot(u8 pos, u16 x, u16 y, u16 width, u16 height){
	hotSpots[pos].x=x;
	hotSpots[pos].y=y;
	hotSpots[pos].width=width;
	hotSpots[pos].height=height;
}

void Wpad_Init(void){
	// Initialise the Wiimotes
	WPAD_Init();
	WPAD_SetIdleTimeout(60*10);
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

	PAD_Init();
}

void Wpad_Finish(void){
	u32 cnt;

	// Disconnect Wiimotes
	for (cnt = 0; cnt < 4; cnt++)
		WPAD_Disconnect(cnt);

	// Shutdown Wiimote subsystem
	WPAD_Shutdown();
}

int Wpad_Scan(void){
	WPAD_ScanPads();
	WPADKeyDown = WPAD_ButtonsDown(WPAD_CHAN_0);

	WPADKeyHeld = WPAD_ButtonsHeld(WPAD_CHAN_0);
	WPAD_SetVRes(WPAD_CHAN_0, Video_GetFBWidth(), Video_GetFBHeight());
	WPAD_IR(WPAD_CHAN_0, &P1Mote);

	int hotSpot=-1;
	int i;
	for(i=0;i<MAXHOTSPOTS;i++){
		if(P1MX>hotSpots[i].x && P1MX<hotSpots[i].x+hotSpots[i].width && P1MY>hotSpots[i].y && P1MY<hotSpots[i].y+hotSpots[i].height){
			hotSpot=i;
			break;
		}
	}


	//WiiMote IR Viewport correction
	P1MX = P1Mote.sx - 150;
	P1MY = P1Mote.sy - 150;
	P1MA = P1Mote.angle;

	return hotSpot;
}

int Wpad_GetWiimoteX(void){
	return P1MX;
}

int Wpad_GetWiimoteY(void){
	return P1MY;
}

int Wpad_GetWiimoteAngle(void){
	return P1MA;
}

int Wpad_GetWiimoteKeyDown(void){
	return WPADKeyDown;
}

int Wpad_GetWiimoteKeyHeld(void){
	return WPADKeyHeld;
}
