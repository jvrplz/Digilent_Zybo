/*------------------------------------------------------------------------------
 *      Módulo SW: Cada vez que se detecta una activación de un switch
 *      se envía un mensaje a una cola con información del switch pulsado
 *-----------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sw.h"
#include "xparameters.h"
#include <stdio.h>
#include <xgpio.h>

/* Definitions for peripheral AXI_GPIO_0 */
#define SWITCHES_DEVICE_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define XPAR_AXI_GPIO_0_BASEADDR 0x41200000
#define XPAR_AXI_GPIO_0_HIGHADDR 0x4120FFFF
#define XPAR_AXI_GPIO_0_INTERRUPT_PRESENT 0
#define XPAR_AXI_GPIO_0_IS_DUAL 0

XGpio GpioSwitches;

void swTask(void *pvParameters);

BaseType_t tid_sw;

QueueHandle_t mid_Queue_sw;

MSGQUEUE_SW_t sw;

int Init_sw(void){
	mid_Queue_sw = xQueueCreate(QUEUE_LENGTH, sizeof(MSGQUEUE_SW_t));

    if (mid_Queue_sw == NULL) {
        xil_printf("Error al crear la cola SW\r\n");
        return -1;
    }

    tid_sw = xTaskCreate(swTask, "SW_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	if(tid_sw != pdPASS){
		return -1;
	}
    return 0;
}

void swTask(void *pvParameters) {
    u32 currentState;
    uint16_t lastValueSent = 0xFFFF; // Un valor inicial distinto a los posibles (SW0, SW3, 0x00)
    InitSwitches();

    while (1) {
        currentState = XGpio_DiscreteRead(&GpioSwitches, 1);

        uint16_t newValue = 0x00;
        // Determinar un único valor en función de los switches
        if (currentState & SW0) {
            newValue = SW0;
        } else if (currentState & SW3) {
            newValue = SW3;
        } else {
            newValue = 0x00; // Ningún switch activo
        }

        // Enviar solo si ha cambiado respecto al último valor enviado
        if (newValue != lastValueSent) {
            sw.value = newValue;
            if (xQueueSend(mid_Queue_sw, &sw, portMAX_DELAY) != pdPASS) {
                xil_printf("Error al enviar el estado de los switches\r\n");
            }
            lastValueSent = newValue;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void InitSwitches(void) {
    int Status;
    Status = XGpio_Initialize(&GpioSwitches, SWITCHES_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("Error al inicializar los switches\r\n");
        return;
    }

    // Configurar el GPIO como entrada
    XGpio_SetDataDirection(&GpioSwitches, 1, 0xFFFFFFFF); // Canal 1 como entrada
}




