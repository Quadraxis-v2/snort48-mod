/*******************************************************************************
 * tools.c
 *
 * Copyright (c) 2009 The Lemon Man
 * Copyright (c) 2009 Nicksasa
 * Copyright (c) 2009 WiiPower
 * Copyright (c) 2025 Quadraxis_v2
 *
 * Distributed under the terms of the GNU General Public License (v2)
 * See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
 *
 ******************************************************************************/

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tools.h"

void *allocate_memory(u32 size){
	return memalign(32, (size+31)&(~31) );
}

/*s32 __FileCmp(const void *a, const void *b){
	dirent_t *hdr1 = (dirent_t *)a;
	dirent_t *hdr2 = (dirent_t *)b;
	
	if (hdr1->type == hdr2->type){
		return strcmp(hdr1->name, hdr2->name);
	}else{
		return 0;
	}
}*/

s32 getdir(char *path, dirent_t **ent, u32 *cnt)
{
	int i, j, k, ret;
	u32 num = 0;

	#ifdef DEBUG_VERSION
		char logMessage[150];
		sprintf(logMessage, "Trying to enter directory: %s", path);
		Log(logMessage, INFO);
	#endif

	ret = ISFS_ReadDir(path, NULL, &num);
	if (ret != ISFS_OK) // Most likely titles of this type don't exist in the system
	{
		#ifdef DEBUG_VERSION
			Log("ISFS_ReadDir returned NOT OK. Directory possibly does not exist.", WARN);
		#endif
		return -1; 
	}

	char ebuf[ISFS_MAXPATH + 1];

	char *nbuf = (char *)allocate_memory((ISFS_MAXPATH + 1) * num);
	if (!nbuf)
	{
		#ifdef DEBUG_VERSION
			Log("Memory allocation error for entry name list", ERROR);
		#endif
		printf("ERROR: could not allocate buffer for name list!\n");
		return -2;
	}

	ret = ISFS_ReadDir(path, nbuf, &num);
	DCFlushRange(nbuf,13*num); //quick fix for cache problems?
	if(ret != ISFS_OK)
	{
		#ifdef DEBUG_VERSION
			char logMessage[150];
			sprintf(logMessage, "Could not get name list! (result: %i)", ret);
			Log(logMessage, ERROR);
		#endif

		printf("ERROR: could not get name list! (result: %d)\n", ret);
		free(nbuf);
		return -3;
	}
	
	*cnt = num;
	
	*ent = allocate_memory(sizeof(dirent_t) * num);
	if (!(*ent))
	{
		#ifdef DEBUG_VERSION
			Log("Memory allocation error for directory entry.", ERROR);
		#endif

		printf("Error: could not allocate buffer\n");
		free(nbuf);
		return -4;
	}

	for(i = 0, k = 0; i < num; i++)
	{	    
		for(j = 0; nbuf[k] != 0; j++, k++)
			ebuf[j] = nbuf[k];
		ebuf[j] = 0;
		k++;

		#ifdef DEBUG_VERSION
			char logMessage[150];
			sprintf(logMessage, "Entry name: %s", ebuf);
			Log(logMessage, DEBUG);
		#endif

		strcpy((*ent)[i].name, ebuf);
	}
	
	//qsort(*ent, *cnt, sizeof(dirent_t), __FileCmp);

	free(nbuf);

	#ifdef DEBUG_VERSION
		sprintf(logMessage, "Entry count for this directory: %i", *cnt);
		Log(logMessage, INFO);
		Log("Exiting getdir()", DEBUG);
	#endif

	return 0;
}

int read_file(char *filepath, u8 **buffer)
{
	int fd, ret;
	fstats *status;

	#ifdef DEBUG_VERSION
		Log("Entering read_file", DEBUG);
		char logMessage[150];
		sprintf(logMessage, "Opening file path = %s", filepath);
		Log(logMessage, INFO);
	#endif

	fd = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (fd < 0)
	{
		#ifdef DEBUG_VERSION
			char logMessage[150];
			sprintf(logMessage, "Error opening file, ret = %i", fd);
			Log(logMessage, ERROR);
		#endif
		return fd;
	}

	status = allocate_memory(sizeof(fstats));
	if (!status)
	{
		#ifdef DEBUG_VERSION
			Log("Memory allocation error for fstats. Out of memory.", ERROR);
		#endif
		return -1;
	}

	#ifdef DEBUG_VERSION
		Log("Getting file stats", DEBUG);
	#endif
	
	ret = ISFS_GetFileStats(fd, status);
	if (ret < 0)
	{
		#ifdef DEBUG_VERSION
			char logMessage[150];
			sprintf(logMessage, "Error getting file stats, ret = %i", ret);
			Log(logMessage, INFO);
		#endif

		ISFS_Close(fd);
		free(status);
		return -2;
	}
	
	*buffer = allocate_memory(status->file_length);
	if (!(*buffer))
	{
		#ifdef DEBUG_VERSION
			Log("Memory allocation error for file buffer", ERROR);
		#endif

		ISFS_Close(fd);
		free(status);
		return -3;
	}

	#ifdef DEBUG_VERSION
		Log("Reading file", DEBUG);
	#endif
		
	ret = ISFS_Read(fd, *buffer, status->file_length);
	if (ret < 0)
	{
		#ifdef DEBUG_VERSION
			char logMessage[150];
			sprintf(logMessage, "Error reading file, ret = %i", ret);
			Log(logMessage, ERROR);
		#endif

		printf("ISFS_Read failed %d\n", ret);
		ISFS_Close(fd);
		free(status);
		free(*buffer);
		return ret;
	}
	ISFS_Close(fd);

	free(status);

	#ifdef DEBUG_VERSION
		Log("Exiting read_file", ERROR);
	#endif

	return ret;
}


int Log(const char* message, Level logLevel)
{
	const char logPath[] = "/snort48log.txt";

	// Use this in case you want to log to SD inside UNEEK

	char messageAligned[strlen(message) + 10] ATTRIBUTE_ALIGN(32);
	switch (logLevel)
	{
		case DEBUG:	sprintf(messageAligned, "[%s] %s\n", "DEBUG", message); 	break;
		case ERROR: sprintf(messageAligned, "[%s] %s\n", "ERROR", message); 	break;
		case INFO: 	sprintf(messageAligned, "[%s] %s\n", "INFO", message);		break;
		case TRACE: sprintf(messageAligned, "[%s] %s\n", "TRACE", message); 	break;
		case WARN: 	sprintf(messageAligned, "[%s] %s\n", "WARN", message); 		break;
	}

	FILE* logFile = fopen(logPath, "a");
	if (!logFile) return -1;

	fprintf(logFile, messageAligned);

	fclose(logFile);

	// Use this in case you want to log to SD inside SNEEK

	/*int fileDescriptor = ISFS_Open(logPath, ISFS_OPEN_WRITE);
	if (fileDescriptor < 0) return fileDescriptor;

	int errorCode = ISFS_Seek(fileDescriptor, 0, 2);
	if (errorCode < 0) 
	{
		ISFS_Close(fileDescriptor);
		return errorCode; 
	}

	if ((errorCode = ISFS_Write(fileDescriptor, messageAligned, strlen(messageAligned))) < 0)
	{
		ISFS_Close(fileDescriptor);
		return errorCode;
	}

	ISFS_Close(fileDescriptor);*/
	return 0;
}
