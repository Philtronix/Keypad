///////////////////////////////////////////////////////////////////////////////
/// @file   usb_hid_keyboard.c
/// @author Phil
///
/// @date   10 Aug 2022
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Header files

#include <stdbool.h>

#include "usb_hid_keyboard.h"

#include "screen.h"
#include "main.h"
#include "CLI.h"
#include "usbd_hid.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usb_device.h"

// https://community.st.com/s/question/0D50X00009Xkh3gSAB/usb-hid-keyboard-leds

///////////////////////////////////////////////////////////////////////////////
// External Variables
///////////////////////////////////////////////////////////////////////////////

extern USBD_HandleTypeDef hUsbDeviceFS;

///////////////////////////////////////////////////////////////////////////////
// Type definitions
///////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint8_t MODIFIER;
	uint8_t RESERVED;
	uint8_t KEYCODE1;
	uint8_t KEYCODE2;
	uint8_t KEYCODE3;
	uint8_t KEYCODE4;
	uint8_t KEYCODE5;
	uint8_t KEYCODE6;
} keyboardHID;

#define KEY_1 0
#define KEY_2 1
#define KEY_3 2
#define KEY_4 3
#define KEY_R 4

///////////////////////////////////////////////////////////////////////////////
// Global Variables
///////////////////////////////////////////////////////////////////////////////

GPIOKEY keys[NUM_KEYS] =
{
	{  SW_1_GPIO_Port,   SW_1_Pin, GPIO_PIN_SET, 0},	// KEY_1
	{  SW_2_GPIO_Port,   SW_2_Pin, GPIO_PIN_SET, 0},	// KEY_2
	{  SW_3_GPIO_Port,   SW_3_Pin, GPIO_PIN_SET, 0},	// KEY_3
	{  SW_4_GPIO_Port,   SW_4_Pin, GPIO_PIN_SET, 0},	// KEY_4
	{SW_TOG_GPIO_Port, SW_TOG_Pin, GPIO_PIN_SET, 0},	// KEY_R
};

keyboardHID keyboardhid = {0,0,0,0,0,0,0,0};

static uint16_t	toggleCount = 0;
static uint16_t	toggleDirection = TOGGLE_DIR_CLOCK;
static uint8_t  HID_buffer[8] = { 0 };

///////////////////////////////////////////////////////////////////////////////
// Local Functions
///////////////////////////////////////////////////////////////////////////////

void USB_Keyboard_SendString(char * s);
void USB_Keyboard_SendChar(char ch);

///////////////////////////////////////////////////////////////////////////////
/// @brief   Scan the keys, recording each of their states
///////////////////////////////////////////////////////////////////////////////
void USB_Keyboard_Scan()
{
	GPIO_PinState	state;
	uint16_t		newCount = 0;

	for (int i = 0; i < NUM_KEYS; i++)
	{
	    state = HAL_GPIO_ReadPin(keys[i].port, keys[i].pin);

	    // Has the key state changed
		if (state != keys[i].state)
	    {
			keys[i].count++;
			keys[i].state = state;

			// Send text on key up
			if (GPIO_PIN_SET == state)
			{
				switch (i)
				{
				case KEY_1:
					USB_Keyboard_SendString("stuff");
					break;

				case KEY_2:
					USB_Keyboard_SendString("wibble");
					break;

				case KEY_3:
					USB_Keyboard_SendString("This is key four");
					break;

				case KEY_4:
					USB_Keyboard_SendString("Hello World");
					break;

				case KEY_R:
					USB_Keyboard_SendString("Rotary");
					break;

				default:
					Message("Unknown key");
					break;

				}
			}
	    }
	}

	newCount = TIM3->CNT;

	if (newCount != toggleCount)
	{
		// Decrease = clockwise
		// Increase = anti-clockwise
		if (newCount > toggleCount)
		{
			if (newCount - toggleCount > 20)
			{
				toggleDirection = TOGGLE_DIR_CLOCK;
				USB_Keyboard_SendString("up");
			}
			else
			{
				toggleDirection = TOGGLE_DIR_ANTI;
				USB_Keyboard_SendString("down");
			}
		}
		else
		{
			if (toggleCount - newCount > 20)
			{
				toggleDirection = TOGGLE_DIR_ANTI;
				USB_Keyboard_SendString("down");
			}
			else
			{
				toggleDirection = TOGGLE_DIR_CLOCK;
				USB_Keyboard_SendString("up");
			}
		}

		toggleCount = newCount;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// @brief   Returns if key is pressed or not
///
/// @param   key - Which key to check
///
/// @return  true  - key is pressed (down)
///          false - key is not pressed (up)
///////////////////////////////////////////////////////////////////////////////
bool USB_IsKeyPressed(int key)
{
	bool retVal = false;

	if (GPIO_PIN_RESET == keys[key].state)
	{
		retVal = true;
	}
	else
	{
		retVal = false;
	}

	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief   Returns the number of times a keys state has changed
///
/// @param   key - Which key to check
///
/// @return  Number of times the state has changed u-down and down-up
///////////////////////////////////////////////////////////////////////////////
int USB_GetKeycount(int key)
{
	return keys[key].count;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief   Returns how often the rotary toggle switch has changed state
///
/// @return  Number of changes
///////////////////////////////////////////////////////////////////////////////
int USB_GetTogglecount()
{
	return toggleCount;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief   Returns which direction the rotary switch last rotated in
///
/// @return  TOGGLE_DIR_CLOCK or TOGGLE_DIR_ANTI
///////////////////////////////////////////////////////////////////////////////
uint8_t USB_GetToggDirection()
{
	return toggleDirection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Send character as a single key press
void USB_Keyboard_SendChar(char ch)
{
	uint8_t ret;

	// Check if lower or upper case
	if(ch >= 'a' && ch <= 'z')
	{
		HID_buffer[0] = 0;
		// convert ch to HID letter, starting at a = 4
		HID_buffer[2] = (uint8_t)(4 + (ch - 'a'));
	}
	else if(ch >= 'A' && ch <= 'Z')
	{
		// Add left shift
		HID_buffer[0] = 2;
		// convert ch to lower case
		ch = ch - ('A'-'a');
		// convert ch to HID letter, starting at a = 4
		HID_buffer[2] = (uint8_t)(4 + (ch - 'a'));
	}
	else // not a letter
	{
		switch(ch)
		{
			case ' ':
				HID_buffer[2] = 44;
				break;
			case '.':
				HID_buffer[2] = 55;
				break;
			case '\n':
				HID_buffer[2] = 40;
				break;
			case '!':
				//combination of shift modifier and key
				HID_buffer[0] = 2;	// shift
				HID_buffer[2] = 30; // number 1
				break;
			case '?':
				//combination of shift modifier and key
				HID_buffer[0] = 2;	// shift
				HID_buffer[2] = 56; // key '/'
				break;
			default:
				HID_buffer[2] = 0;
		}
	}

	// press keys
	ret = USBD_HID_SendReport(&hUsbDeviceFS, HID_buffer, 8);
	if(ret != HAL_OK)
	{
		Error_Handler();
	}
	HAL_Delay(15);

	// release keys
	HID_buffer[0] = 0;
	HID_buffer[2] = 0;
	ret = USBD_HID_SendReport(&hUsbDeviceFS, HID_buffer, 8);
	if(ret != HAL_OK)
	{
		Error_Handler();
	}
	HAL_Delay(15);
}

// Send string as letters
void USB_Keyboard_SendString(char * s)
{
	uint8_t i = 0;

	while(*(s+i))
	{
		USB_Keyboard_SendChar(*(s+i));
		i++;
	}
}


