#ifndef __DHT11_H_
#define __DHT11_H_

#include "qpc.h"

typedef struct
{
	float temperature;
	float humidity;
} Dht11Result;

Dht11Result *GetDht11Result();
void Dht11Ctor(void);
extern QActive *g_Dht11;

#endif // !__DHT11_H_