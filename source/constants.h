/*
	SNORT48+ - constants.h
	Constants for SNORT48+
*/

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#define MAX_WIDTH		116
#define MAX_HEIGHT		92

#define FIRSTCOL		116
#define FIRSTROW		96

#define SEPARATECOL		136
#define SEPARATEROW		112

#define COLS			4
#define ROWS			3
#define PAGES			4
#define MAXCHANNELS 	COLS*ROWS*PAGES

#define ARROWSX			32
#define ARROWSY			206

#define MAXHOTSPOTS		(COLS*ROWS)+2 //2=left arrow+right arrow
#define HOTSPOT_LEFT	MAXHOTSPOTS-2
#define HOTSPOT_RIGHT	MAXHOTSPOTS-1

#define IPLSAVE_PATH		"/title/00000001/00000002/data/iplsave.bin"
#endif