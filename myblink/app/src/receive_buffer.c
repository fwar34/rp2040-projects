#include "receive_buffer.h"
#include <string.h>
#include <stdlib.h>

static ReceiveBuffer receiveBuffer;

ReceiveBuffer *GetReceiveBuffer()
{
	return &receiveBuffer;
}

void RecvBufInit(ReceiveBuffer *receiveBuffer, uint8_t *data, uint16_t elementSize, uint16_t elementCount)
{
	receiveBuffer->data = data;
	receiveBuffer->elementCount = elementCount;
	receiveBuffer->elementSize = elementSize;
	receiveBuffer->readIndex = 0;
	receiveBuffer->writeIndex = 0;
}

uint16_t RecvBufGetSize(ReceiveBuffer *receiveBuffer)
{
	int32_t ret = receiveBuffer->writeIndex - receiveBuffer->readIndex;
	if (ret < 0) {
		ret += (receiveBuffer->elementCount + 1);
	}

	return ret;
}

bool RecvBufIsEmpty(ReceiveBuffer *receiveBuffer)
{
	return RecvBufGetSize(receiveBuffer) == 0;
}

bool RecvBufIsFull(ReceiveBuffer *receiveBuffer)
{
	return RecvBufGetSize(receiveBuffer) == receiveBuffer->elementCount;
}

bool RecvBufWrite(ReceiveBuffer *receiveBuffer, uint8_t *data, bool isBlock)
{
	if () {

	}
}

void RecvBufRead(ReceiveBuffer* receiveBuffer, uint16_t consumeLen)
{
	receiveBuffer->consumePointer = receiveBuffer->data + consumeLen;
	receiveBuffer->dataLen -= consumeLen;
}

void RecvBufClear(ReceiveBuffer *receiveBuffer)
{
	memset(receiveBuffer, 0, sizeof(*receiveBuffer));
	receiveBuffer->capacity = RECEIVE_BUFFER_LENGTH;
}
