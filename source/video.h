#ifndef _VIDEO_H_
#define _VIDEO_H_

void Video_Init(void);
int Video_GetFBWidth(void);
int Video_GetFBHeight(void);
void Video_Finish(void);
void Video_Print(int,int,const char*, f32, u32);
void Video_DrawBackground(void);
void Video_DrawCursor(void);
void Video_DrawPageInfo(u8, u8, u8);

void Video_DrawBanner(const char*, const char*,int,int, u8);
void Video_DrawEmptyBanner(int,int, u8);

#endif