#include "qp_port.h"

typedef struct TableTag {
    QActive super;

    uint8_t fork[];
    
} Table;