/*******************************************************************************
 * tools.c
 *
 * Copyright (c) 2009 The Lemon Man
 * Copyright (c) 2009 Nicksasa
 * Copyright (c) 2009 WiiPower
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

s32 getdir(char *path, dirent_t **ent, u32 *cnt){
	int i, j, k, ret;
	u32 num = 0;

	ret = ISFS_ReadDir(path, NULL, &num);
	if(ret != ISFS_OK){
		printf("Error: could not get dir entry count! (result: %d)\n", ret);
		return -1;
	}

	char ebuf[ISFS_MAXPATH + 1];

	char *nbuf = (char *)allocate_memory((ISFS_MAXPATH + 1) * num);
	if(nbuf == NULL){
		printf("ERROR: could not allocate buffer for name list!\n");
		return -2;
	}

	ret = ISFS_ReadDir(path, nbuf, &num);
	DCFlushRange(nbuf,13*num); //quick fix for cache problems?
	if(ret != ISFS_OK){
		printf("ERROR: could not get name list! (result: %d)\n", ret);
		free(nbuf);
		return -3;
	}
	
	*cnt = num;
	
	*ent = allocate_memory(sizeof(dirent_t) * num);
	if(*ent==NULL){
		printf("Error: could not allocate buffer\n");
		free(nbuf);
		return -4;
	}

	for(i = 0, k = 0; i < num; i++){	    
		for(j = 0; nbuf[k] != 0; j++, k++)
			ebuf[j] = nbuf[k];
		ebuf[j] = 0;
		k++;

		strcpy((*ent)[i].name, ebuf);
	}
	
	//qsort(*ent, *cnt, sizeof(dirent_t), __FileCmp);

	free(nbuf);
	return 0;
}

int read_file(char *filepath, u8 **buffer){
	int fd, ret;
	fstats *status;

	fd = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (fd < 0){
		return fd;
	}

	status = allocate_memory(sizeof(fstats));
	if (status == NULL){
		return -1;
	}
	
	ret = ISFS_GetFileStats(fd, status);
	if (ret < 0){
		ISFS_Close(fd);
		free(status);
		return -2;
	}
	
	*buffer = allocate_memory(status->file_length);
	if (*buffer == NULL){
		ISFS_Close(fd);
		free(status);
		return -3;
	}
		
	ret = ISFS_Read(fd, *buffer, status->file_length);
	if (ret < 0){
		printf("ISFS_Read failed %d\n", ret);
		ISFS_Close(fd);
		free(status);
		free(*buffer);
		return ret;
	}
	ISFS_Close(fd);

	free(status);

	return ret;
}
