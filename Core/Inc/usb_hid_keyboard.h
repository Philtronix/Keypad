#ifndef __USB_HID_KEYBOARD_H
#define __USB_HID_KEYBOARD_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define NUM_KEYS 5

#define TOGGLE_DIR_CLOCK	1
#define TOGGLE_DIR_ANTI		2

typedef struct tagGPIOKEY
{
	GPIO_TypeDef*	port;
	uint16_t		pin;
	GPIO_PinState	state;
	int				count;
} GPIOKEY;


void    USB_Keyboard_Scan();
bool    USB_IsKeyPressed(int key);
int     USB_GetKeycount(int key);
int     USB_GetTogglecount();
uint8_t USB_GetToggDirection();

//void USB_Keyboard_SendChar(char ch);
//void USB_Keyboard_SendString(char * s);

#endif

