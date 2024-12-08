#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Bluetooth.h"
#include "xparameters.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sleep.h"
#include "xil_cache.h"
#include "xuartps.h"
typedef XUartPs SysUart;
#define SysUart_Send            XUartPs_Send
#define SysUart_Recv            XUartPs_Recv
#define SYS_UART_DEVICE_ID      XPAR_PS7_UART_1_DEVICE_ID
#define BT2_UART_AXI_CLOCK_FREQ 100000000

XUartNs550_Config BT2_Config = {0, 0, 0, 0};

PmodBT2 myDevice;
SysUart myUart;

void Bluetooth_ReceiveTask(void *pvParameters);
void Bluetooth_TransmitTask(void *pvParameters);

BaseType_t tid_BlueRX;
BaseType_t tid_BlueTX;

QueueHandle_t mid_Queue_TX_Blue;
QueueHandle_t mid_Queue_RX_Blue;

MSGQUEUE_BLU_TX_t bluetx;
MSGQUEUE_BLU_RX_t bluerx;

int Init_Bluetooth(void){
	mid_Queue_TX_Blue = xQueueCreate(QUEUE_LENGTH, sizeof(MSGQUEUE_BLU_TX_t));
	mid_Queue_RX_Blue = xQueueCreate(QUEUE_LENGTH, sizeof(MSGQUEUE_BLU_RX_t));

    if (mid_Queue_TX_Blue == NULL || mid_Queue_RX_Blue == NULL) {
        xil_printf("Error al crear las colas bluetooth\r\n");
        return -1;
    }

    tid_BlueRX = xTaskCreate(Bluetooth_ReceiveTask, "BT_Receive", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
	if(tid_BlueRX != pdPASS){
		return -1;
	}

	tid_BlueTX = xTaskCreate(Bluetooth_TransmitTask, "BT_Transmit", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
	if(tid_BlueTX != pdPASS){
		return -1;
	}
    return 0;
}

void Bluetooth_ReceiveTask(void *pvParameters) {
    int n;
    int buf_index = 0;
    Bluetooth_Initialize();

    while (1) {
        // Leer datos del Bluetooth
        n = BT2_RecvData(&myDevice, (u8 *)&bluerx.string[buf_index], 1);
        if (n > 0) {
            // Procesar datos recibidos
            if (bluerx.string[buf_index] == '\r' || bluerx.string[buf_index] == '\n') {
                if (buf_index > 0) {
                	bluerx.string[buf_index] = '\0'; // Terminar la cadena
                    xQueueSend(mid_Queue_RX_Blue, &bluerx, portMAX_DELAY);
                    buf_index = 0; // Reiniciar buffer
                }
            } else {
                buf_index++;
                if (buf_index >= QUEUE_ITEM_SIZE) {
                    buf_index = 0; // Reiniciar buffer en caso de desbordamiento
                }
            }
        } else {

        	vTaskDelay(pdMS_TO_TICKS(10));

        }
        //taskYIELD();
        //suspended
    }
}

void Bluetooth_TransmitTask(void *pvParameters) {
	Bluetooth_Initialize();
	xil_printf("\r\nConexion Bluetooth establecida\r\n");
    while (1) {
        if (xQueueReceive(mid_Queue_TX_Blue, &bluetx, portMAX_DELAY)) {
            SendInChunks(&myDevice, bluetx.string); // Envia el mensaje en partes
        }
    	taskYIELD();
    }
}

void SysUartInit() {
    XUartPs_Config *myUartCfgPtr;
    myUartCfgPtr = XUartPs_LookupConfig(SYS_UART_DEVICE_ID);
    XUartPs_CfgInitialize(&myUart, myUartCfgPtr, myUartCfgPtr->BaseAddress);
}

void Bluetooth_Initialize() {
    SysUartInit();
    BT2_Begin(&myDevice, XPAR_PMODBT2_0_AXI_LITE_GPIO_BASEADDR, XPAR_PMODBT2_0_AXI_LITE_UART_BASEADDR, BT2_UART_AXI_CLOCK_FREQ, 115200);
}

// Función para enviar en fragmentos de 16 bytes
void SendInChunks(PmodBT2 *InstancePtr, const char *message) {
    size_t len = strlen(message);
    size_t sent = 0;

    while (sent < len) {
        size_t chunk_size = (len - sent > 16) ? 16 : (len - sent); // Máximo 16 bytes por fragmento
        BT2_SendData(InstancePtr, (u8 *)(message + sent), chunk_size);
        sent += chunk_size;
        vTaskDelay(pdMS_TO_TICKS(10)); // Retardo entre fragmentos
    }
}

void BT2_Begin(PmodBT2 *InstancePtr, u32 GPIO_Address, u32 UART_Address, u32 AXI_ClockFreq, u32 Uart_Baud) {
   BT2_Config.BaseAddress = UART_Address;
   BT2_Config.InputClockHz = AXI_ClockFreq;
   BT2_Config.DefaultBaudRate = Uart_Baud;
   InstancePtr->GpioBaseAddr = GPIO_Address;
   InstancePtr->AXI_ClockFreq = AXI_ClockFreq;

   Xil_Out32(InstancePtr->GpioBaseAddr + 4, 0x1);
   Xil_Out32(InstancePtr->GpioBaseAddr, 0x0); // set CTS to 0

   XUartNs550_CfgInitialize(&InstancePtr->Uart, &BT2_Config, BT2_Config.BaseAddress);
   XUartNs550_SetOptions(&InstancePtr->Uart, XUN_OPTION_FIFOS_ENABLE);
}

int BT2_RecvData(PmodBT2 *InstancePtr, u8 *Data, int nData) {
   return XUartNs550_Recv(&InstancePtr->Uart, Data, nData);
}

int BT2_SendData(PmodBT2 *InstancePtr, u8 *Data, int nData) {
   return XUartNs550_Send(&InstancePtr->Uart, Data, nData);
}

void BT2_ChangeBaud(PmodBT2 *InstancePtr, int baud) {
   XUartNs550_SetBaud(InstancePtr->Uart.BaseAddress, InstancePtr->AXI_ClockFreq,
         baud);
}
