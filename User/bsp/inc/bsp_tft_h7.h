/*
*********************************************************************************************************
*
*	模块名称 : STM32H7驱动液晶
*	文件名称 : bsp_tft_h7.h
*	版    本 : V2.0
*	说    明 : 头文件
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef _BSP_TFT_H7_H
#define _BSP_TFT_H7_H

#define LCD_LAYER_1     0x0000		/* 顶层 */
#define LCD_LAYER_2		0x0001		/* 第2层 */

/* 可供外部模块调用的函数 */
void LCDH7_InitHard(void);
void LCDH7_GetChipDescribe(char *_str);
void LCDH7_DispOn(void);
void LCDH7_DispOff(void);
void LCDH7_ClrScr(uint16_t _usColor);
void LCDH7_PutPixel(uint16_t _usX, uint16_t _usY, uint16_t _usColor);
uint16_t LCDH7_GetPixel(uint16_t _usX, uint16_t _usY);
void LCDH7_DrawLine(uint16_t _usX1 , uint16_t _usY1 , uint16_t _usX2 , uint16_t _usY2 , uint16_t _usColor);
void LCDH7_DrawPoints(uint16_t *x, uint16_t *y, uint16_t _usSize, uint16_t _usColor);
void LCDH7_DrawRect(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth, uint16_t _usColor);
void LCDH7_DrawCircle(uint16_t _usX, uint16_t _usY, uint16_t _usRadius, uint16_t _usColor);
void LCDH7_DrawBMP(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth, uint16_t *_ptr);
void LCDH7_FillRect(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth, uint16_t _usColor);

void LCDH7_SetDirection(uint8_t _dir);

void LCDH7_SetLayer(uint8_t _ucLayer);

void LCDH7_SetDispWin(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth);
void LCDH7_QuitWinMode(void);
void _LCD_DrawCamera16bpp(int x, int y, uint16_t * p, int xSize, int ySize, int SrcOffLine);
#endif


