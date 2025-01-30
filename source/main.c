/* == SORT SNEEK ==
 * main.c - Manages Sort Sneek menu
 * Copyright (C) 2011  Marc R., Juan de la Cruz C.G.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation version 2 or later.
 *
 * See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>   

#include <grrlib.h>

#include <wiiuse/wpad.h>
#include <math.h>
#include <ogc/lwp_watchdog.h>

#include <sys/dir.h> // for dirent_t

#include <fat.h> 

#include "video.h"
#include "wpad.h"
#include "constants.h"
#include "tools.h"
#include "md5.h"

#define MII_CHANNEL			0x0001000248414341
#define PHOTO_CHANNEL		0x0001000248414141
#define PHOTO_CHANNEL_11	0x0001000248415941
#define SHOP_CHANNEL		0x0001000248414241

#define DISC_CHANNEL		0x0000000000000000
#define SORT_CHANNEL		0x00010001534f5254

typedef struct{
	u64 idInt;
	char id[5];
	char* name;
	u8 inMenu;
} channel;
static channel* channels = 0;
static u16 nChannels = 1;

static s16 order[MAXCHANNELS];
static s16 moving = -1;
static u8 page = 0;


void __Error_Message(const char* text, int err) {
	int i;
	char errMsg[16];

	sprintf(errMsg,"(ret=%d)",err);

	#ifdef DEBUG_VERSION
		Log(text, ERROR);
	#endif

	while (1) 
	{
		Video_DrawBackground();

		GRRLIB_Rectangle(0, 104, 640, 240, RGBA(255,255,255,128), 1);
		for(i = 0; i < 16; i++)
		{
			GRRLIB_Rectangle(0, 104 - 16 + i, 640, 1, RGBA(0, 0, 0, i * 2), 1);
			GRRLIB_Rectangle(0, 104 + 240 + 16 - i, 640, 1, RGBA(0, 0, 0, i * 2), 1);
		}

		Video_Print(40, 190, text, 1.5 ,RGBA(0, 0, 0, 255));
		Video_Print(40, 220, errMsg, 1, RGBA(255, 0, 0, 255));

		Wpad_Scan();

		Video_Print(40, 285, "(press any button to continue)", 0.8, RGBA(64, 64, 64, 255));

		Video_DrawCursor();

		GRRLIB_Render();  // Render the frame buffer to the TV    

		if (Wpad_GetWiimoteKeyDown() & WPAD_BUTTON_A) break;
	}
}


int __GetTitlesFromNAND(void)
{
	#ifdef DEBUG_VERSION
		Log("Entering __GetTitlesFromNAND", DEBUG);
	#endif

	u32 maxnum = 0, entryCount = 0;
	dirent_t* list = 0;
	char path[102] = "";

	s32 language = CONF_GetLanguage();

	#ifdef DEBUG_VERSION
		char logMessage[100];
		sprintf(logMessage, "CONF_GetLanguage returns %i", language);
		Log(logMessage, INFO);
	#endif

	for (int i = 1; i <= 4; i++)
	{
		#ifdef DEBUG_VERSION
			char logMessage[100];
			sprintf(logMessage, "Entering loop, i = %i", i);
			Log(logMessage, INFO);
		#endif

		if (i != 3)
		{
			sprintf(path, "/title/0001000%i", i);
			int ret = getdir(path, &list, &entryCount);
			if (ret >= 0)
			{
				maxnum += entryCount;

				channel* channelCopy = channels;
				channels = realloc(channels, sizeof(channel) * (maxnum + 1)); //+1 (disc channel)
				if (!channels)
				{
					#ifdef DEBUG_VERSION
						Log("Memory allocation error for channels. Out of memory.", ERROR);
					#endif

					if (channelCopy) free(channelCopy);
					free(list);
					__Error_Message("Out of memory.", -1);
					return -1;
				}

				for(int j = 0; j < entryCount; j++)
				{
					//char *out;
					u8* buffer = allocate_memory(0x1E4 + 4);
					if (!buffer)
					{
						#ifdef DEBUG_VERSION
							Log("Memory allocation error for TMD buffer. Out of memory.", ERROR);
						#endif
						__Error_Message("Out of memory.", -3);
					}

					// Try to read from banner.bin first
					sprintf(path, "/title/0001000%i/%s/content/title.tmd", i, list[j].name);

					#ifdef DEBUG_VERSION
						char logMessage[150];
						sprintf(logMessage, "Opening TMD, path = %s", path);
						Log(logMessage, INFO);
					#endif

					int cfd = ISFS_Open(path, ISFS_OPEN_READ);
					if (cfd >= 0)
					{
						#ifdef DEBUG_VERSION
							Log("Reading TMD into buffer.", DEBUG);
						#endif

						ret = ISFS_Read(cfd, buffer, 0x1E4 + 4);
						if (ret < 0)
						{
							#ifdef DEBUG_VERSION
								Log("ISFS_Read failed for TMD.", ERROR);
							#endif

							__Error_Message("ISFS_Read for title failed.", ret);
							ISFS_Close(cfd);
							free(buffer);
							buffer = 0;
							continue;
						}
						ISFS_Close(cfd);	

						//this is very dirty!
						int realBannerContent = *(int*)(buffer + 0x1E4);
						free(buffer);
						buffer = 0;

						//try to read banner content
						sprintf(path, "/title/0001000%i/%s/content/%08x.app", i, list[j].name, realBannerContent);

						#ifdef DEBUG_VERSION
							char logMessage[150];
							sprintf(logMessage, "Opening title content = %s", path);
							Log(logMessage, INFO);
						#endif

						cfd = ISFS_Open(path, ISFS_OPEN_READ);
						if (cfd < 0) 
						{
							#ifdef DEBUG_VERSION
								char logMessage[150];
								sprintf(logMessage, "ISFS_Open failed, ret = %i. Title not installed?", cfd);
								Log(logMessage, WARN);
							#endif
							continue;	//the title is not installed after all
						}
						else
						{
							buffer = allocate_memory(0x54);
							if (!buffer)
							{
								#ifdef DEBUG_VERSION
									Log("Memory allocation error for title name buffer. Out of memory.", ERROR);
								#endif
								__Error_Message("Out of memory.", -3);
							}

							#ifdef DEBUG_VERSION
								Log("Seeking title name position.", DEBUG);
							#endif

							ret = ISFS_Seek(cfd, 0x9D + language * 0x54, 0);
							if (ret < 0)
							{
								#ifdef DEBUG_VERSION
									char logMessage[150];
									sprintf(logMessage, "ISFS_Seek failed, ret = %i", ret);
									Log(logMessage, ERROR);
								#endif

								__Error_Message("ISFS_Seek for a channel failed.", ret);
								ISFS_Close(cfd);
								free(buffer);
								buffer = 0;
								continue;
							}

							#ifdef DEBUG_VERSION
								Log("Reading title name.", DEBUG);
							#endif

							ret = ISFS_Read(cfd, buffer, 0x54);
							if (ret < 0)
							{
								#ifdef DEBUG_VERSION
									char logMessage[150];
									sprintf(logMessage, "ISFS_Read failed, ret = %i", ret);
									Log(logMessage, ERROR);
								#endif

								__Error_Message("ISFS_Read for a channel failed.", ret);
								ISFS_Close(cfd);
								free(buffer);
								buffer = 0;
								continue;
							}

							if (buffer[0] == '\0') // Title does not have our language, use default language
							{
								ret = ISFS_Seek(cfd, 0x9D, 0);
								if (ret < 0)
								{
									__Error_Message("ISFS_Seek for a channel failed.", ret);
									ISFS_Close(cfd);
									free(buffer);
									buffer = 0;
									continue;
								}

								ret = ISFS_Read(cfd, buffer, 0x54);
								if (ret < 0)
								{
									__Error_Message("ISFS_Read for a channel failed.", ret);
									ISFS_Close(cfd);
									free(buffer);
									buffer = 0;
									continue;
								}
							}

							ISFS_Close(cfd);

							//Calculate channel title length
							int k = 0;
							for (k = 0; buffer[k * 2] != 0x00; k++);
							
							s32 length = k;

							channels[nChannels].name = malloc(length + 1);
							if (!channels[nChannels].name)
							{
								#ifdef DEBUG_VERSION
									char logMessage[150];
									sprintf(logMessage, "Memory allocation error for channel name, length = %i", length);
									Log(logMessage, ERROR);
								#endif

								__Error_Message("Allocating memory for buffer failed.", -2);
								free(buffer);
								continue;
							}

							for (k = 0; k < length; k++) channels[nChannels].name[k] = buffer[k * 2];
							channels[nChannels].name[length] = '\0';

							free(buffer);

							//get ID
							channels[nChannels].idInt = TITLE_ID(0x00010000 + i, strtol(list[j].name, 0, 16));

							//get string ID
							strncpy(channels[nChannels].id, (char*)&channels[nChannels].idInt + 4, 4);
							channels[nChannels].id[4] = '\0';
							channels[nChannels].inMenu = false;

							#ifdef DEBUG_VERSION
								char logMessage[150];
								sprintf(logMessage, "Title name = %s, Title ID = %s, number of channels so far = %i", 
									channels[nChannels].name, channels[nChannels].id, nChannels);
								Log(logMessage, INFO);
							#endif

							++nChannels;
						}
					}
					else
					{
						#ifdef DEBUG_VERSION
							char logMessage[150];
							sprintf(logMessage, "Error opening TMD, ret = %i", cfd);
							Log(logMessage, ERROR);
						#endif
					}
				}
			}
		}
		free(list);
	}

	strcpy(channels[0].id, "0000");
	channels[0].name = "Disc Channel";
	channels[0].idInt = DISC_CHANNEL;
	channels[0].inMenu = true;

	#ifdef DEBUG_VERSION
		Log("Exiting __GetTitlesFromNAND", DEBUG);
	#endif

	return 0;
}



void __SortChannelsByIPLSAVE(void)
{
	#ifdef DEBUG_VERSION
		Log("Entering __SortChannelsByIPLSAVE", DEBUG);
	#endif

	iplsave* ipl = allocate_memory(sizeof(iplsave));
	if (!ipl)
	{
		#ifdef DEBUG_VERSION
			Log("Memory allocation error for iplsave.bin. Out of memory.", ERROR);
		#endif
		__Error_Message("Out of memory.", -3);
	}

	int ret = read_file(IPLSAVE_PATH, (void *)&ipl);
	if(ret < 0)
	{
		#ifdef DEBUG_VERSION
			Log("Error reading iplsave.bin.", ERROR);
		#endif

		__Error_Message("Error reading IPL file.", ret);
		return;
	}
	else if (ret != sizeof(iplsave))
	{
		#ifdef DEBUG_VERSION
			Log("Invalid IPL file size. Use a 4.x System Menu", ERROR);
		#endif

		__Error_Message("Error invalid IPL file size. Use a 4.x SM.", ret);
		return;
	}

	for (int i = 0; i < MAXCHANNELS; i++)
	{
		iplsave_entry* entry;
		order[i] = -1;

		entry = &ipl->channels[i];
		if (entry->channel_type != 0)
		{
			for(int j = 0; j < nChannels; j++)
			{
				if (entry->title_id == channels[j].idInt)
				{
					order[i] = j;
					channels[j].inMenu = true;
					break;
				}
			}
		}
	}
	free(ipl);

	#ifdef DEBUG_VERSION
		Log("Exiting __SortChannelsByIPLSAVE", DEBUG);
	#endif
}


void __SaveChangesToIPL(void)
{
    //unsigned char md5[16];
	int i, ret, iplsave_fd;
	iplsave iplsave_buf;

	iplsave_buf.magic[0] = 'R';
	iplsave_buf.magic[1] = 'I';
	iplsave_buf.magic[2] = 'P';
	iplsave_buf.magic[3] = 'L';

	iplsave_buf.filesize = sizeof(iplsave);

	iplsave_buf.unknown[0] = 0x00;
	iplsave_buf.unknown[1] = 0x00;
	iplsave_buf.unknown[2] = 0x00;
	iplsave_buf.unknown[3] = 0x03;
	iplsave_buf.unknown[4] = 0x00;
	iplsave_buf.unknown[5] = 0x00;
	iplsave_buf.unknown[6] = 0x00;
	iplsave_buf.unknown[7] = 0x00;

	//Check if SORT_CHANNEL exists
	for (i = 0; i < MAXCHANNELS; i++) if (channels[order[i]].idInt == SORT_CHANNEL) break;
	if(i == MAXCHANNELS)
		for (i = 0; i < nChannels; i++) if (channels[i].idInt == SORT_CHANNEL) order[MAXCHANNELS-1] = i;

	//Force disc channel
	for (i = 0; i < MAXCHANNELS; i++) if (channels[order[i]].idInt == DISC_CHANNEL) break;
	if(i == MAXCHANNELS) order[0] = 0;


	for(i = 0; i < MAXCHANNELS; i++)
	{
		int game = order[i];
		if (game == -1)
		{
			iplsave_buf.channels[i].channel_type = 0x00;
			iplsave_buf.channels[i].secondary_type = 0x00;
			iplsave_buf.channels[i].flags = 0x0000;
			iplsave_buf.channels[i].title_id = 0x0000000000000000;
		}
		else
		{
			//CHANNEL TYPE: 01=disc channel ,  03=other channels
			//FLAGS:        000f=disc       ,  000e=normal
			if (channels[game].idInt == DISC_CHANNEL)
			{
				iplsave_buf.channels[i].channel_type = 0x01;
				iplsave_buf.channels[i].flags = 0x000f;
				iplsave_buf.channels[i].title_id = DISC_CHANNEL;
			}
			else
			{
				iplsave_buf.channels[i].channel_type = 0x03;
				iplsave_buf.channels[i].flags = 0x000e;
				iplsave_buf.channels[i].title_id = channels[game].idInt;
			}

			//SECONDARY TYPE: 00=normal , 01=disc,mii,shop,photo channel
			if (channels[game].idInt == DISC_CHANNEL || channels[game].idInt == MII_CHANNEL ||
				channels[game].idInt == SHOP_CHANNEL || channels[game].idInt == PHOTO_CHANNEL || 
				channels[game].idInt == PHOTO_CHANNEL_11) iplsave_buf.channels[i].secondary_type = 0x01;
			else iplsave_buf.channels[i].secondary_type = 0x00;

		}
		iplsave_buf.channels[i].unknown[0] = 0x00;
		iplsave_buf.channels[i].unknown[1] = 0x00;
		iplsave_buf.channels[i].unknown[2] = 0x00;
		iplsave_buf.channels[i].unknown[3] = 0x00;
	}

    
    //md5_bytes(md5, &iplsave_buf, sizeof(iplsave)-0x10);
	//for(i=0; i<16; i++)
	//	iplsave_buf.md5_sum[i]=md5[i];
    md5_bytes((u8*)&iplsave_buf.md5_sum, (unsigned char*)&iplsave_buf, sizeof(iplsave) - 0x10);



	// Save new iplsave.bin
	ret = ISFS_Initialize();
	if(ret)
	{
		__Error_Message("Error initializing ISFS.", ret);
		return;
	}

	iplsave_fd = IOS_Open(IPLSAVE_PATH, IPC_OPEN_WRITE);
	if(iplsave_fd<0)
	{
		ISFS_Deinitialize();
		__Error_Message("Can't open iplsave.bin.", iplsave_fd);
		return;
	}

	ret = IOS_Write(iplsave_fd, &iplsave_buf, sizeof(iplsave));
	if (ret != sizeof(iplsave))
	{
		IOS_Close(iplsave_fd);
		ISFS_Deinitialize();
		__Error_Message("Could not write to iplsave.bin.", ret);
		return;
	}
       
	IOS_Close(iplsave_fd);
	ISFS_Deinitialize();
}

void __Draw_Banner(int game, bool selected, int x, int y)
{
	if(selected)
	{
		if(game == -1) Video_DrawEmptyBanner(x, y, moving >- 1 ? 3: 2);
		else Video_DrawBanner(channels[game].id, channels[game].name, x, y, moving>-1? 3: 1);
	}
	else
	{
		if(game == -1) Video_DrawEmptyBanner(x, y, 0);
		else Video_DrawBanner(channels[game].id, channels[game].name, x, y, 0);
	}
}

s32 __ChannelIDCmp(const void *a, const void *b){
	char str1[128];
	char str2[128];

	channel* ch1 = &channels[*((int*)a)];
	channel* ch2 = &channels[*((int*)b)];	
	sprintf(str1, "%c%s", ch1->id[0], ch1->name);
	sprintf(str2, "%c%s", ch2->id[0], ch2->name);

	return strcmp(str1, str2);
}

#define VALIDROWS 15
int __Select_Game(void){
	int validChannels[nChannels];
	int validPage, maxValidPages, selected;
	u32 button;
	int ret=-1;
	int nValid=0;
	char tempString[128];

	bool isPhotoChannelPresent = false;
	int photoChannelIndex = 0;
	bool isPhotoChannel11Present = false;
	
	for(int i = 0; i < nChannels; i++)
	{
		if (!isPhotoChannelPresent && !strcmp(channels[i].id, "HAAA")) 
		{
			isPhotoChannelPresent = true;
			photoChannelIndex = i;
		}
		else 
		{
			if (!isPhotoChannel11Present && !strcmp(channels[i].id, "HAYA")) isPhotoChannel11Present = true;
			if (!channels[i].inMenu)
			{
				validChannels[nValid] = i;
				++nValid;
			}
		}
	}
	if (isPhotoChannelPresent && !isPhotoChannel11Present && !channels[photoChannelIndex].inMenu)
	{
		validChannels[nValid] = photoChannelIndex;
		++nValid;
	}

	if(nValid == 0) return -1;

	qsort(&validChannels, nValid, sizeof(int), __ChannelIDCmp);

	maxValidPages = (nValid / VALIDROWS) + (nValid % VALIDROWS != 0);
	//__Error_Message("nValid", nValid);
	//__Error_Message("pages", maxValidPages);

	validPage = 0;
	selected = 0;

	for(;;)
	{
		Video_DrawBackground();
		GRRLIB_Rectangle(0, 0, 640, 480, 0x000000b0, 1);

		for(int i = 0; i<VALIDROWS && validPage*VALIDROWS+i<nValid; i++){
			sprintf(tempString, "%s %s", channels[validChannels[validPage*VALIDROWS+i]].id, channels[validChannels[validPage*VALIDROWS+i]].name);
			Video_Print(48, 48+i*20, tempString, 1.5, selected==i? 0x0000FFff : 0xFFFFFFff);
		}

		sprintf(tempString, "%d/%d", validPage+1, maxValidPages);
		Video_Print(48, 384, tempString, 1.5, 0xFFFFFFff);
		Wpad_Scan();
		button=Wpad_GetWiimoteKeyDown();

		if(button & WPAD_BUTTON_LEFT && validPage>0){
			validPage--;
		}if(button & WPAD_BUTTON_RIGHT && validPage+1<maxValidPages){
			validPage++;
		}if(button & WPAD_BUTTON_UP && selected>0){
			selected--;
		}if(button & WPAD_BUTTON_DOWN && selected+1<VALIDROWS){
			selected++;
		}if(button & WPAD_BUTTON_A){
			ret=validChannels[validPage*VALIDROWS+selected];
			break;
		}else if(button & WPAD_BUTTON_B || button & WPAD_BUTTON_HOME){
			break;
		}
		while((validPage*VALIDROWS+selected)>nValid-1)
			selected--;
		GRRLIB_Render();
	}

	return ret;
}


int main(int argc, char **argv)
{
	int i, j, hot, ret;

	Video_Init();


	Video_DrawBackground();
	GRRLIB_Rectangle(0, 0, 640, 480, 0x000000b0, 1);
	Video_Print(264 + 2, 208 + 2, "Loading...", 1.5, 0x000000ff);
	Video_Print(264, 208, "Loading...", 1.5, 0xFFFFFFff);
	GRRLIB_Render();

	Wpad_Init();

	ret = ISFS_Initialize();
	if(ret < 0)
	{
		__Error_Message("Error initializing ISFS.", ret);
		goto out;
	}
	#ifdef DEBUG_VERSION
		if (!fatInitDefault()) __Error_Message("wtf", -1);
		remove("snort48log.txt");
		/*ISFS_Delete("/snort48log.txt");
		int errorCode = ISFS_CreateFile("/snort48log.txt", 0, 3, 3, 3);
		if (errorCode < 0) __Error_Message("Error creating log file", errorCode);
		else
		{
			errorCode = Log("ISFS initialized.", DEBUG);
			if (errorCode < 0) __Error_Message("USB logging error", errorCode);
		}*/
	#endif

	ret = __GetTitlesFromNAND();
	if(ret < 0)
	{
		#ifdef DEBUG_VERSION
			char logMessage[150];
			sprintf(logMessage, "__GetTitlesFromNAND exited with error, ret = %i", ret);
			Log(logMessage, ERROR);
		#endif

		__Error_Message("Error getting title list.", ret);
		goto out;
	}

	__SortChannelsByIPLSAVE();

	ISFS_Deinitialize();

	#ifdef DEBUG_VERSION
		Log("Adding hotspots.", DEBUG);
	#endif

	//ADD HOTSPOTS
	for(i = 0;i < ROWS; i++)
	{
		for(j = 0; j < COLS; j++)
		{
			u8 pos = i * COLS + j;
			Wpad_AddHotSpot(pos, (FIRSTCOL + j * SEPARATECOL) - MAX_WIDTH / 2, 
				(FIRSTROW + i * SEPARATEROW) - MAX_HEIGHT / 2, MAX_WIDTH, MAX_HEIGHT);
		}
		Wpad_AddHotSpot(HOTSPOT_LEFT, 0, 172, 46, 80);
		Wpad_AddHotSpot(HOTSPOT_RIGHT, 640 - 46, 172, 46, 80);
	}

	#ifdef DEBUG_VERSION
		Log("Finished adding hotspots.", DEBUG);
	#endif

	int wait = 0;

	while (1)
	{
		hot = Wpad_Scan();

		Video_DrawBackground();

		Video_DrawPageInfo((page > 0) + (page > 0 && hot == HOTSPOT_LEFT), 
			(page + 1 < PAGES) + (page + 1 < PAGES && hot == HOTSPOT_RIGHT), page);
		for (i = 0; i < ROWS; i++)
		{
			for (j = 0; j < COLS; j++)
			{
				if (moving != (page * ROWS * COLS) + i * COLS + j)
				{
					int x = FIRSTCOL + j * SEPARATECOL;
					int y = FIRSTROW + i * SEPARATEROW;
					bool selected = (i * COLS + j == hot);
					int game = order[(page * ROWS * COLS) + i * COLS+j];
					__Draw_Banner(game, selected,x,y);
				}
			}
		}

		//Draw pointer or moving banner
		if(moving == -1)
			Video_DrawCursor();
		else
			__Draw_Banner(order[moving], false, Wpad_GetWiimoteX(), Wpad_GetWiimoteY());


		/*char strHotSpot[128];
		sprintf(strHotSpot,"HOTSPOT %d",hot);
		Video_Print(10,410,strHotSpot,22,0x00FF00FF);*/

		u32 button = Wpad_GetWiimoteKeyDown();
		u32 buttonHeld = Wpad_GetWiimoteKeyHeld();

		if(hot > -1 && hot < ROWS * COLS && (buttonHeld & WPAD_BUTTON_B) && (button & WPAD_BUTTON_A))
		{
			int game = (page * ROWS * COLS) + hot;
			moving = game;
		}
		else if (moving > -1 && !((buttonHeld & WPAD_BUTTON_B) && (buttonHeld & WPAD_BUTTON_A)))
		{
			if(hot > -1 && hot < ROWS * COLS)
			{
				s16 copy0 = order[moving];
				s16 copy1 = order[(page * ROWS * COLS) + hot];
				order[moving] = copy1;
				order[(page * ROWS * COLS) + hot] = copy0;
			}
			moving = -1;
		}
		if (button & WPAD_BUTTON_HOME) break;

		if (hot==HOTSPOT_LEFT || hot==HOTSPOT_RIGHT) wait++;
		if (page > 0 && ((moving > -1 && hot == HOTSPOT_LEFT && wait == 100) || (button & WPAD_BUTTON_MINUS) || 
			(hot == HOTSPOT_LEFT && (button & WPAD_BUTTON_A))))
		{
			page--;
			wait = 0;
		}
		if (page < PAGES-1 && ((button & WPAD_BUTTON_PLUS) || (hot == HOTSPOT_RIGHT && (button & WPAD_BUTTON_A)) || 
			(moving > -1 && hot == HOTSPOT_RIGHT && wait == 100)))
		{
			page++;
			wait = 0;
		}
		if (moving == -1 && (hot >= 0 && hot < ROWS * COLS) && (button & WPAD_BUTTON_A))
		{
			int pos = (page * ROWS * COLS) + hot;
			int game = order[pos];

			if (game == -1)
			{
				order[pos] = __Select_Game();
				if (order[pos] != -1) channels[order[pos]].inMenu = true;
			}
			else
			{
				channels[game].inMenu = false;
				order[pos] = -1;
			}
		}

		GRRLIB_Render();  // Render the frame buffer to the TV    
	}

	Video_DrawBackground();
	GRRLIB_Rectangle(0, 0, 640, 480, 0x000000b0, 1);
	Video_Print(264+2, 208+2, "Saving...", 1.5, 0x000000ff);
	Video_Print(264, 208, "Saving...", 1.5, 0xFFFFFFff);
	GRRLIB_Render();

	__SaveChangesToIPL();


out:
	Wpad_Finish();
	Video_Finish();

	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	exit(0);
}
