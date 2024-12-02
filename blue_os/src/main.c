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
QueueHandle_t recvQueue;
QueueHandle_t sendQueue;

// Prototipos de tareas
void Bluetooth_ReceiveTask(void *pvParameters);
void Bluetooth_TransmitTask(void *pvParameters);
void Bluetooth_ProcessTask(void *pvParameters);

static TaskHandle_t xTxTask;
static TaskHandle_t xRxTask;
static TaskHandle_t xDataTask;

void Bluetooth_Initialize();
void SysUartInit();
void ProcessCommand(const char *command, char *response, int *enabled, int *position);
void SendInChunks(PmodBT2 *InstancePtr, const char *message);

PmodBT2 myDevice;
SysUart myUart;

int main(void) {
    // Inicializar Bluetooth
    Bluetooth_Initialize();
    xil_printf("\r\nBluetooth inicializado con FreeRTOS\r\n");

    // Crear colas
    recvQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    sendQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);


    if (recvQueue == NULL || sendQueue == NULL) {
        xil_printf("Error al crear las colas\r\n");
        return -1;
    }

    // Crear tareas
    xTaskCreate(Bluetooth_ReceiveTask, "BT_Receive", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &xRxTask);
    xTaskCreate(Bluetooth_TransmitTask, "BT_Transmit", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, &xTxTask);
    xTaskCreate(Bluetooth_ProcessTask, "BT_Process", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDataTask);

    // Iniciar el scheduler
    vTaskStartScheduler();

    while (1); // Nunca debería llegar aquí
    return 0;
}

void Bluetooth_Initialize() {
    SysUartInit();
    BT2_Begin(
        &myDevice,
        XPAR_PMODBT2_0_AXI_LITE_GPIO_BASEADDR,
        XPAR_PMODBT2_0_AXI_LITE_UART_BASEADDR,
        BT2_UART_AXI_CLOCK_FREQ,
        115200
    );
}

void Bluetooth_ReceiveTask(void *pvParameters) {
    char buffer[QUEUE_ITEM_SIZE];
    int n;
    int buf_index = 0;

    while (1) {
        // Leer datos del Bluetooth
        n = BT2_RecvData(&myDevice, (u8 *)&buffer[buf_index], 1);
        if (n != 0) {
            // Procesar datos recibidos
            if (buffer[buf_index] == '\r' || buffer[buf_index] == '\n') {
                if (buf_index > 0) {
                    buffer[buf_index] = '\0'; // Terminar la cadena
                    xQueueSend(recvQueue, buffer, portMAX_DELAY); // Enviar a la cola
                    buf_index = 0; // Reiniciar buffer
                }
            } else {
                buf_index++;
                if (buf_index >= QUEUE_ITEM_SIZE) {
                    buf_index = 0; // Reiniciar buffer en caso de desbordamiento
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Pequeño retardo para evitar bucles apretados
    }
}

void Bluetooth_TransmitTask(void *pvParameters) {
    char message[QUEUE_ITEM_SIZE];

    while (1) {
        if (xQueueReceive(sendQueue, message, portMAX_DELAY)) {
            SendInChunks(&myDevice, message); // Envia el mensaje en partes
        }
    }
}

void Bluetooth_ProcessTask(void *pvParameters) {
    char command[QUEUE_ITEM_SIZE];
    char response[QUEUE_ITEM_SIZE];
    int position = 10;
    int enabled = 0;

    while (1) {
        if (xQueueReceive(recvQueue, command, portMAX_DELAY)) {
            // Procesar el comando y obtener la respuesta
            ProcessCommand(command, response, &enabled, &position);

            xQueueSend(sendQueue, response, portMAX_DELAY);
            xil_printf("%s\r\n", response);
        }
    }
}

void ProcessCommand(const char *command, char *response, int *enabled, int *position) {
    if (strcmp(command, "e") == 0) {
        *enabled = 1;
        snprintf(response, 64, "Habilitado\r\n");
    } else if (strcmp(command, "d") == 0) {
        *enabled = 0;
        snprintf(response, 64, "Inhabilitado\r\n");
    } else if (command[0] == '+' && strlen(command) == 1) {
        if (*enabled) {
            *position += 5;
            if (*position > 90) *position = 90;
            snprintf(response, 64, "Posicion actual = %d\r\n", *position);
        } else {
            snprintf(response, 64, "Sistema inhabilitado\r\n");
        }
    } else if (command[0] == '-' && strlen(command) == 1) {
        if (*enabled) {
            *position -= 5;
            if (*position < 10) *position = 10;
            snprintf(response, 64, "Posicion actual = %d\r\n", *position);
        } else {
            snprintf(response, 64, "Sistema inhabilitado\r\n");
        }
    } else if (strlen(command) == 2 && isdigit(command[0]) && isdigit(command[1])) {
        int new_position = atoi(command);
        if (new_position >= 10 && new_position <= 90) {
            if (*enabled) {
                *position = new_position;
                snprintf(response, 64, "Posicion actual = %d\r\n", *position);
            } else {
                snprintf(response, 64, "Sistema inhabilitado\r\n");
            }
        } else {
            snprintf(response, 64, "Mensaje incorrecto\r\n");
        }
    } else {
        snprintf(response, 64, "Mensaje incorrecto\r\n");
    }
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

void SysUartInit() {
#ifdef __MICROBLAZE__
    XUartLite_Initialize(&myUart, SYS_UART_DEVICE_ID);
#else
    XUartPs_Config *myUartCfgPtr;
    myUartCfgPtr = XUartPs_LookupConfig(SYS_UART_DEVICE_ID);
    XUartPs_CfgInitialize(&myUart, myUartCfgPtr, myUartCfgPtr->BaseAddress);
#endif
}
