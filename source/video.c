/*
	SNORT48+ - video.c
	Manages video functions for Snort48+.
	* Copyright (C) 2011  Marc R., Juan de la Cruz C.G.
	*
	* This program is free software; you can redistribute it and/or
	* modify it under the terms of the GNU General Public License
	* as published by the Free Software Foundation version 2 or later.
	*
	* See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
*/

#include <grrlib.h>
#include <stdlib.h>

#include "constants.h"
#include "wpad.h"
//#include "video.h"

#include "cursor_png.h"
#include "borders_png.h"
#include "background_png.h"
#include "mask_png.h"
#include "arrows_png.h"
#include "pages_png.h"
#include "font_png.h"

#define OPAQUE			0xFFFFFFFF

// Initialize general variables
extern GXRModeObj *rmode;

// Prepare Graphics
GRRLIB_texImg* GFX_Cursor;
GRRLIB_texImg* GFX_Borders;
GRRLIB_texImg* GFX_Fondo;
GRRLIB_texImg* GFX_Mask;
GRRLIB_texImg* GFX_Arrows;
GRRLIB_texImg* GFX_Pages;
GRRLIB_texImg* GFX_Font;




void Video_Init(void) {
	// Initialise the Graphics & Video subsystem
	GRRLIB_Init();

	// Load textures
	GFX_Cursor		=	GRRLIB_LoadTexturePNG(cursor_png);
	GFX_Borders		=	GRRLIB_LoadTexturePNG(borders_png);
    					GRRLIB_InitTileSet(GFX_Borders, 136, 128, 0);
	GFX_Fondo		=	GRRLIB_LoadTexturePNG(background_png);
	GFX_Mask		=	GRRLIB_LoadTexturePNG(mask_png);

	GFX_Arrows		=	GRRLIB_LoadTexturePNG(arrows_png);
    					GRRLIB_InitTileSet(GFX_Arrows, 20, 36, 0);
	GFX_Pages		=	GRRLIB_LoadTexturePNG(pages_png);
    					GRRLIB_InitTileSet(GFX_Pages, 56, 20, 0);

    //Prepare font
	GFX_Font		=	GRRLIB_LoadTexturePNG(font_png);
    					GRRLIB_InitTileSet(GFX_Font, 8, 16, 32);

	GRRLIB_Settings.antialias = true;
   
	// Set handles
	GRRLIB_SetMidHandle(GFX_Cursor, true);
	GRRLIB_SetMidHandle(GFX_Borders, true);
	GRRLIB_SetMidHandle(GFX_Mask, true);
	GRRLIB_SetMidHandle(GFX_Arrows, true);
}

int Video_GetFBWidth(void){
	return rmode->fbWidth;
}

int Video_GetFBHeight(void){
	return rmode->efbHeight;
}
void Video_Finish(void){
	GRRLIB_Exit(); // Clear the memory allocated by GRRLIB

	// Free all memory used by textures
	GRRLIB_FreeTexture(GFX_Cursor);
	GRRLIB_FreeTexture(GFX_Borders);
	GRRLIB_FreeTexture(GFX_Fondo);
	GRRLIB_FreeTexture(GFX_Mask);
	GRRLIB_FreeTexture(GFX_Arrows);
	GRRLIB_FreeTexture(GFX_Pages);
	GRRLIB_FreeTexture(GFX_Font);
}

void Video_Print(int x,int y,const char* str, f32 tamano, u32 color){
	GRRLIB_Printf(x, y, GFX_Font, color, tamano, str);
}




void Video_DrawBackground(void){
	GRRLIB_DrawImg(0, 0, GFX_Fondo, 0, 1, 1, OPAQUE);
}

void Video_DrawCursor(void){
	int x=Wpad_GetWiimoteX();
	int y=Wpad_GetWiimoteY();
	int angle=Wpad_GetWiimoteAngle();

	GRRLIB_DrawImg(x+4, y+2, GFX_Cursor, angle, 1, 1, 0x00000060);
	GRRLIB_DrawImg(x, y, GFX_Cursor, angle, 1, 1, OPAQUE);

	/*
	char strPointerPosition[32];
	sprintf(strPointerPosition,"POS: x=%d  y=%d",x,y);
	GRRLIB_PrintfTTF(10, 440, myFont, strPointerPosition, 22, 0xFF000080);
	*/
}

void Video_DrawPageInfo(u8 left, u8 right, u8 page){
	int i;
	GRRLIB_DrawTile(ARROWSX, ARROWSY, GFX_Arrows, 0, 1, 1, OPAQUE, left);
	GRRLIB_DrawTile(640-ARROWSX, ARROWSY, GFX_Arrows, 0, -1, -1, OPAQUE, right);

	for(i=0; i<PAGES; i++)
		GRRLIB_DrawTile(196+64*i, 388, GFX_Pages, 0, 1, 1, i==page? OPAQUE : 0xffffff80, i);
}




void Video_DrawBanner(const char* id, const char* title, int x, int y, u8 selected)
{
	u32 color = 0x1c2f45e0;
	if (id[0] == 'W') color = 0x3d265ce0;
	else if (id[0] == 'F') color = 0x5c2626e0;
	else if (id[0] == 'J') color = 0x26585ce0;
	else if (id[0] == 'N') color = 0x265c2ee0;
	else if (id[0] == 'H' || id[0] == 'D') color = 0x060606e0;
	else if (id[0] == 'S' || id[1] == 'O' || id[2] == 'R' || id[3] == 'T') color = 0x002affe0;
	
	//515c26
	//5c4726
	GRRLIB_DrawImg(x, y, GFX_Mask, 0, 1, 1, color);
	Video_Print(x - 10, y - 16, id, 0.6, 0xFFFFFF80);

	// Adjust title name position
	char* titleCopy = "";
	strcpy(titleCopy, title);

	char* token = strtok(titleCopy, " ");
	char aux[32] = "";
	strcpy(aux, token);
	token = strtok(0, " ");

	int i = 0;
	while (token)
	{
		if (strlen(aux) + 1 + strlen(token) <= 13) strcat(strcat(aux, " "), token);
		else 
		{
			Video_Print(x - 40 + (13 - strlen(aux)) * 3, y + 8 + i * 16, aux, 0.8, 0xFFFFFFe0);
			strcpy(aux, token);
			++i;
		}
		token = strtok(0, " ");
	}
	Video_Print(x - 40 + (13 - strlen(aux)) * 3, y + 8 + i * 16, aux, 0.8, 0xFFFFFFe0);

	GRRLIB_DrawTile(x, y, GFX_Borders, 0, 1, 1, OPAQUE, selected);
}


void Video_DrawEmptyBanner(int x,int y, u8 selected){
	GRRLIB_DrawImg(x, y, GFX_Mask, 0, 1, 1, 0xa8a8a8e0);
	GRRLIB_DrawTile(x, y, GFX_Borders, 0, 1, 1, OPAQUE, selected);
}
