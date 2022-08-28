///////////////////////////////////////////////////////////////////////////////
/// @file       CircularBuffer.cpp
/// @copyright  Copyright (c) Philtronix ltd - All rights Reserved
///             Unauthorised copying of this file, via any medium is strictly
///             prohibited.
///
/// @brief      Store data in a circular buffer
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////
#include "CircularBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Public Function definitions
///////////////////////////////////////////////////////////////////////////////


inline uint32_t CircularBuffer_StoredItems(CircularBuffer_t* buffer)
{
    if (buffer->writeIndex >= buffer->readIndex)
    {
        return buffer->writeIndex - buffer->readIndex;
    }
    else
    {
        return buffer->maxItems - buffer->readIndex + buffer->writeIndex;
    }
}

bool CircularBuffer_Init(CircularBuffer_t* buffer, uint8_t* data, uint32_t size)
{
    bool ret = true;

    buffer->maxItems   = size;
    buffer->writeIndex = 0;
    buffer->readIndex  = 0;
    buffer->items      = data;

    if (buffer->items == NULL)
    {
        ret = false;
    }

    return ret;
}

void CircularBuffer_DeInit(CircularBuffer_t* buffer)
{
    buffer->maxItems   = 0;
    buffer->writeIndex = 0;
    buffer->readIndex  = 0;

    if (buffer->items != NULL)
    {
        free(buffer->items);
    }
}

bool CircularBuffer_WriteByte(CircularBuffer_t* buffer, uint8_t b)
{
    bool ret = false;

    if (CircularBuffer_StoredItems(buffer) < buffer->maxItems - 1)
    {
        buffer->items[buffer->writeIndex++] = b;
        if (buffer->writeIndex >= buffer->maxItems)
        {
            buffer->writeIndex = 0;
        }
        ret = true;
    }

    return ret;
}

bool CircularBuffer_ReadByte(CircularBuffer_t* buffer, uint8_t* b)
{
    bool ret = true;
    if (CircularBuffer_StoredItems(buffer) == 0)
    {
        ret = false;
    }
    else
    {
        (*b) = buffer->items[buffer->readIndex++];
        if (buffer->readIndex >= buffer->maxItems)
        {
            buffer->readIndex = 0;
        }
    }

    return ret;
}
