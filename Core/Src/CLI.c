///////////////////////////////////////////////////////////////////////////////
/// @file       CLI.cpp
/// @copyright  Copyright (c) Philtronix ltd - All rights Reserved
///             Unauthorised copying of this file, via any medium is strictly
///             prohibited.
///
/// @brief      Command line interface
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////
#include "CLI.h"
#include "screen.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "CircularBuffer.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////
#define MAX_REC_CHAR			100

#define GOTOXY(x,y)     "\033[x;yH"     // Esc[Line;ColumnH
#define CR				'\r'
#define LF				'\n'
#define DEL				127
#define ESC				27				// Quit display mode
#define NUM_CMDS	    3

///////////////////////////////////////////////////////////////////////////////
// Type definitions
///////////////////////////////////////////////////////////////////////////////
typedef void (*clifunc)();

typedef struct tagCOMMAND
{
	char	text[20];
	clifunc	func;
} COMMAND;

///////////////////////////////////////////////////////////////////////////////
// Private Function declarations
///////////////////////////////////////////////////////////////////////////////

// CLI Functions
static void Help(void);
static void Test1(void);
static void Test2(void);
static void Test3(void);

uint32_t RxBytesAvailable();
void     SendData(const char *data, uint32_t length);
uint32_t StartTransmit(void);
bool     ReadByte(uint8_t *data);
void     EraseOldUser();


///////////////////////////////////////////////////////////////////////////////
// Variable Definitions
///////////////////////////////////////////////////////////////////////////////
char	gszReceiveBuffer[MAX_REC_CHAR + 2];
uint8_t	gNumChar = 0;
uint8_t	bLineEnd = 0;
char    cliBuffer[100] = {0};
uint8_t	cliIndex = 0;
uint8_t Rx_data;
bool    gotData = false;

static bool isTransmitting = false;

CircularBuffer_t txBuffer;
CircularBuffer_t rxBuffer;
uint8_t          UserRxBufferFS[RX_BUFFER_SIZE];
uint8_t          UserTxBufferFS[TX_BUFFER_SIZE];

COMMAND cmds[NUM_CMDS] =
{
	{"test1", Test1},
	{"test2", Test2},
	{"test3", Test3},
};

extern UART_HandleTypeDef huart2;

///////////////////////////////////////////////////////////////////////////////
// Public Function definitions
///////////////////////////////////////////////////////////////////////////////
void Prompt(void)
{
	OutputAt(22, 3, ">");
}

void Message(const char *text)
{
	char szBuffer[] = "                                                                            ";
	int  len;

	// Merge text
	len = strlen(text);
	memcpy(szBuffer, text, len);

	OutputAt(23, 3, szBuffer);
}

void CLI_Init(void)
{
	HAL_UART_Receive_IT(&huart2, &Rx_data, 1);

	CircularBuffer_Init(&txBuffer, UserRxBufferFS, RX_BUFFER_SIZE);
	CircularBuffer_Init(&rxBuffer, UserTxBufferFS, TX_BUFFER_SIZE);
}

void Output(const char *format, ...)
{
	va_list		args;
	char		szBuffer[100] = {0};
	uint32_t	length = 0;

	va_start(args, format);
	length = sprintf(szBuffer, format, args);
	va_end(args);

	SendData(szBuffer, length);
}

void OutputAt(int x, int y, const char *format, ...)
{
	va_list		args;
	char		szBuffer[200] = {0};
	char		szText[200] = {0};
	uint32_t	length;

	// [ *l ; *c H	Move cursor to line *l, column *c
	// [ *l ; *c f	Move curosr to line *l, column *c
	sprintf(szBuffer, "\033[%d;%dH", x, y);
	va_start(args, format);
	vsprintf(szText, format, args);
	va_end(args);

	strcat(szBuffer, szText);
	length = strlen(szBuffer);

	SendData(szBuffer, length);
}

void SendData(const char *data, uint32_t length)
{
	for (uint32_t i = 0; i < length; i++)
	{
		CircularBuffer_WriteByte(&txBuffer, data[i]);
	}

	StartTransmit();
}

// Called by Main()
void CLI_Update(void)
{
	uint8_t data;

	while (RxBytesAvailable() > 0)
	{
		ReadByte(&data);
		CLI_ProcessNewData(data);
	}
}

void CLI_ProcessNewData(uint8_t data)
{
	char szBuffer[200] = {0};

	// Save the new character
	cliBuffer[cliIndex] = data;

	// End of line ?
	if (CR == cliBuffer[cliIndex] ||
		LF == cliBuffer[cliIndex])
	{
		cliBuffer[cliIndex] = 0;

		// Which command ?
		if ((0 == strcmp("help", cliBuffer)) ||
		    (0 == strcmp("?", cliBuffer)))
		{
			Help();
		}
		else
		{
			int match = -1;
			for (int i = 0; i < NUM_CMDS; i++)
			{
				if (0 == strcmp(cmds[i].text, cliBuffer))
				{
					match = i;
				}
			}

			if (match >= 0)
			{
				clifunc func = cmds[match].func;
				(*func)();
			}
			else
			{
				sprintf(szBuffer, "Unrecognised command \"%s\"", cliBuffer);
				Message(szBuffer);
			}
		}

		// Erase old user input
		EraseOldUser();

		cliIndex = 0;
	}
	// Backspace
	else if (DEL == data)
	{
		Output("%c", cliBuffer[cliIndex]);
		cliIndex--;
		cliBuffer[cliIndex] = 0;
	}
	// Escape - Clear screen
	else if (ESC == data)
	{
		RefreshScreen();
	}
	else
	{
		cliIndex++;

		char szBuffer[20] = {0};

		sprintf(szBuffer, "%c", data);

		OutputAt(22, 3 + cliIndex, szBuffer);
	}

	// Update the screen
    ScreenUpdate();
}

void EraseOldUser()
{
	char szBuffer[100] = {0};

	// Erase old user input
	for (int i = 0; i < cliIndex; i++)
	{
		szBuffer[i] = ' ';
	}
	szBuffer[cliIndex] = 0;
	OutputAt(22, 4, szBuffer);
}

uint32_t RxBytesAvailable()
{
	return (CircularBuffer_StoredItems(&rxBuffer));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// Copy received byte to buffer
	CircularBuffer_WriteByte(&rxBuffer, Rx_data);

	// Set up the next read
	HAL_UART_Receive_IT(&huart2, &Rx_data, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	isTransmitting = false;
	StartTransmit();
}

bool ReadByte(uint8_t *data)
{
	return CircularBuffer_ReadByte(&rxBuffer, data);
}

///////////////////////////////////////////////////////////////////////////////
// Private Function definitions
///////////////////////////////////////////////////////////////////////////////

static void Help(void)
{
	Output("Commands :\r\n");

	for (int i = 0; i < NUM_CMDS; i++)
	{
		Output("%s\r\n", cmds[i].text);
	}
}

static void Test1(void)
{
	Output("Test one\r\n");

	Output("Test one - [done]\r\n");
}

static void Test2(void)
{
	Output("Test two\r\n");

	Output("Test two - [done]\r\n");
}

static void Test3(void)
{
	Output("Test three\r\n");

	Output("Test three - [done]\r\n");
}

uint32_t StartTransmit(void)
{
	uint32_t storredItems = CircularBuffer_StoredItems(&txBuffer);
	uint32_t bytesWritten = 0;

	if (!isTransmitting && storredItems > 0)
	{
		for (bytesWritten = 0; (bytesWritten < storredItems) && (bytesWritten < sizeof(UserTxBufferFS - 1)); bytesWritten++)
		{
			CircularBuffer_ReadByte(&txBuffer, &UserTxBufferFS[bytesWritten]);
		}

		if (HAL_OK == HAL_UART_Transmit_IT(&huart2, UserTxBufferFS, bytesWritten))
		{
			isTransmitting = true;
		}
		else
		{
			bytesWritten = 0;
		}
	}

	return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////

/*
uint8_t rot_get_state()
{
	return (uint8_t)((HAL_GPIO_ReadPin(ROT_B_GPIO_Port, ROT_B_Pin) << 1) |
                     (HAL_GPIO_ReadPin(ROT_A_GPIO_Port, ROT_A_Pin)));
}

// External interrupts from rotary encoder
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == ROT_A_Pin || GPIO_Pin == ROT_B_Pin)
	{
		rot_new_state = rot_get_state();

		// Check transition
		if (rot_old_state == 3 && rot_new_state == 2)
		{
			// 3 -> 2 transition
			rot_cnt++;
		}
		else if (rot_old_state == 2 && rot_new_state == 0)
		{
			// 2 -> 0 transition
			rot_cnt++;
		}
		else if (rot_old_state == 0 && rot_new_state == 1)
		{
			// 0 -> 1 transition
			rot_cnt++;
		}
		else if (rot_old_state == 1 && rot_new_state == 3)
		{
			// 1 -> 3 transition
			rot_cnt++;
		}
		else if (rot_old_state == 3 && rot_new_state == 1)
		{
			// 3 -> 1 transition
			rot_cnt--;
		}
		else if (rot_old_state == 1 && rot_new_state == 0)
		{
			// 1 -> 0 transition
			rot_cnt--;
		}
		else if (rot_old_state == 0 && rot_new_state == 2)
		{
			// 0 -> 2 transition
			rot_cnt--;
		}
		else if (rot_old_state == 2 && rot_new_state == 3)
		{
			// 2 -> 3 transition
			rot_cnt--;
		}

		rot_old_state = rot_new_state;
	}
}
*/

/////////////////////////////////


