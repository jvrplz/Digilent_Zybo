#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ALS.h"
#include "xparameters.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sleep.h"
#include "xil_cache.h"
#include "xuartps.h"

#ifdef __MICROBLAZE__
#include "xuartlite.h"
typedef XUartLite SysUart;
#define SysUart_Send            XUartLite_Send
#define SysUart_Recv            XUartLite_Recv
#define SYS_UART_DEVICE_ID      XPAR_AXI_UARTLITE_0_DEVICE_ID
#define BT2_UART_AXI_CLOCK_FREQ XPAR_CPU_M_AXI_DP_FREQ_HZ
#else
#include "xuartps.h"
typedef XUartPs SysUart;
#define SysUart_Send            XUartPs_Send
#define SysUart_Recv            XUartPs_Recv
#define SYS_UART_DEVICE_ID      XPAR_PS7_UART_1_DEVICE_ID
#define BT2_UART_AXI_CLOCK_FREQ 100000000
#endif

#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE 32 // Tamaño máximo de un mensaje

// Colas para comunicación entre tareas
QueueHandle_t lightQueue;

void Main_Task(void *pvParameters);
void ALS_Task(void *pvParameters);

// Prototipos de tareas
void ALS_Task(void *pvParameters);
void Main_Task(void *pvParameters);

static TaskHandle_t xMainTask;
static TaskHandle_t xLightTask;

void SysUartInit();

PmodALS ALS;
SysUart myUart;

int main(void) {
    // Inicializar Bluetooth
    ALS_begin(&ALS, XPAR_PMODALS_0_AXI_LITE_SPI_BASEADDR);
    usleep(1000);
    xil_printf("PmodALS inicializado\r\n");

    lightQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE); // Cola para valores de luminosidad
    if (lightQueue == NULL) {
        xil_printf("Error al crear la cola\r\n");
        return -1;
    }

    xTaskCreate(ALS_Task, "ALS_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xLightTask);
    xTaskCreate(Main_Task, "Main_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &xMainTask);

    // Iniciar el scheduler
    vTaskStartScheduler();

    while (1); // Nunca debería llegar aquí
    return 0;
}


// Tarea de medición del PmodALS
void ALS_Task(void *pvParameters) {
    u8 light;

    while (1) {
        // Leer la luminosidad del sensor
        light = ALS_read(&ALS);

        // Enviar el valor a la cola
        if (xQueueSend(lightQueue, &light, portMAX_DELAY) != pdPASS) {
            xil_printf("Error al enviar el valor a la cola\r\n");
        }

        // Retardo para la próxima medición
        vTaskDelay(pdMS_TO_TICKS(100)); // 100 ms
    }
}

// Tarea principal para gestión futura de la luminosidad
void Main_Task(void *pvParameters) {
    u8 light;

    while (1) {
        // Recibir el valor de luminosidad desde la cola
        if (xQueueReceive(lightQueue, &light, portMAX_DELAY) == pdPASS) {
            // Imprimir el valor recibido (para pruebas iniciales)
            xil_printf("Luminosidad recibida: %d\n\r", light);

            // Aquí se puede agregar la lógica futura para gestionar el valor
        }
    }
}

void SysUartInit() {
#ifdef __MICROBLAZE__
    XUartLite_Initialize(&myUart, SYS_UART_DEVICE_ID);
#else
    XUartPs_Config *myUartCfgPtr;
    myUartCfgPtr = XUartPs_LookupConfig(SYS_UART_DEVICE_ID);
    XUartPs_CfgInitialize(&myUart, myUartCfgPtr, myUartCfgPtr->BaseAddress);
#endif
}
