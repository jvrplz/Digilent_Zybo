#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "xil_types.h"
#include "xstatus.h"
#include "xuartns550.h"

#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE 32

typedef struct PmodBT2 {
    u32 AXI_ClockFreq;
    u32 GpioBaseAddr;
    XUartNs550 Uart;
} PmodBT2;

typedef struct {
	char string[QUEUE_ITEM_SIZE];
} MSGQUEUE_BLU_TX_t;

typedef struct {
	char string[QUEUE_ITEM_SIZE];
} MSGQUEUE_BLU_RX_t;

extern QueueHandle_t mid_Queue_TX_Blue;
extern QueueHandle_t mid_Queue_RX_Blue;

int Init_Bluetooth(void);

void BT2_Begin(PmodBT2 *InstancePtr, u32 GPIO_Address, u32 UART_Address, u32 AXI_ClockFreq, u32 Uart_Baud);
int BT2_RecvData(PmodBT2 *InstancePtr, u8 *Data, int nData);
int BT2_SendData(PmodBT2 *InstancePtr, u8 *Data, int nData);
void BT2_ChangeBaud(PmodBT2 *InstancePtr, int baud);
void SysUartInit();
void Bluetooth_Initialize();
void SendInChunks(PmodBT2 *InstancePtr, const char *message);


#endif // BLUETOOTH_H
