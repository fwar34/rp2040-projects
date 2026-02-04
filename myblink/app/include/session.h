#ifndef __SESSION_H_
#define __SESSION_H_

#include <stdint.h>

#define RECV_BUFFER_LEN 2048

typedef struct {
    uint8_t id;
    char recvBuf[RECV_BUFFER_LEN];
    bool headerParseComplete;
} Session;

void InitSession(Session *session);

#endif // !__SESSION_H_