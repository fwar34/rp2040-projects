/*
 * receive_buffer.h
 *
 *  Created on: Mar 22, 2025
 *      Author: fwar3
 */

#ifndef INC_RECEIVE_BUFFER_H_
#define INC_RECEIVE_BUFFER_H_

#include <inttypes.h>
#include <stdbool.h>

#define RECEIVE_BUFFER_LENGTH 1024

typedef struct {
	uint8_t *data;
	uint16_t elementSize;
	uint16_t elementCount;
	uint16_t writeIndex;
	uint16_t readIndex;
} ReceiveBuffer;

ReceiveBuffer *GetReceiveBuffer();
void RecvBufInit(ReceiveBuffer *receiveBuffer, uint8_t *data, uint16_t elementSize, uint16_t elementCount);
bool IsEmpty(ReceiveBuffer *receiveBuffer);
void ConsumeData(ReceiveBuffer* receiveBuffer, uint16_t consumeLen);
void Clear(ReceiveBuffer *receiveBuffer);

#endif /* INC_RECEIVE_BUFFER_H_ */
