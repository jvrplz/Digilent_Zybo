#ifndef ALS_H
#define ALS_H

#include "xil_types.h"
#include "xspi.h"
#include "xspi_l.h"
#include "xstatus.h"

#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE 32

typedef struct PmodALS {
   XSpi ALSSpi;
} PmodALS;

typedef struct {
	uint8_t light;
} MSGQUEUE_ALS_t;

extern QueueHandle_t mid_Queue_ALS;
extern TaskHandle_t xLightTask;

int Init_ALS(void);

XStatus ALS_begin(PmodALS *InstancePtr, u32 SPI_Address);
XStatus ALS_SPIInit(XSpi *SpiInstancePtr);
u8 ALS_read(PmodALS *InstancePtr);

#endif // ALS_H
