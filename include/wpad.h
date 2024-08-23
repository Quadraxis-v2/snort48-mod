#ifndef _WPAD_H_
#define _WPAD_H_

void Wpad_Init(void);
void Wpad_Finish(void);
int Wpad_Scan(void);
int Wpad_GetWiimoteX(void);
int Wpad_GetWiimoteY(void);
int Wpad_GetWiimoteAngle(void);
int Wpad_GetWiimoteKeyDown(void);
int Wpad_GetWiimoteKeyHeld(void);
void Wpad_AddHotSpot(u8, u16, u16, u16, u16);

#endif