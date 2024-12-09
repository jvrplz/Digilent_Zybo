/*
    Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
    Copyright (C) 2012 - 2018 Xilinx, Inc. All Rights Reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal in
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
    the Software, and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software. If you wish to use our Amazon
    FreeRTOS name, please do so in a fair use way that does not cause confusion.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    http://www.FreeRTOS.org
    http://aws.amazon.com/freertos


    1 tab == 4 spaces!
*/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include <stdio.h>
#include "xil_printf.h"
#include <xgpio.h>

/* Definitions for peripheral AXI_GPIO_0 */
#define SWITCHES_DEVICE_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define XPAR_AXI_GPIO_0_BASEADDR 0x41200000
#define XPAR_AXI_GPIO_0_HIGHADDR 0x4120FFFF
#define XPAR_AXI_GPIO_0_INTERRUPT_PRESENT 0
#define XPAR_AXI_GPIO_0_IS_DUAL 0

XGpio GpioSwitches;

QueueHandle_t switchQueue;

void SwitchReaderTask(void *pvParameters);
void SwitchProcessorTask(void *pvParameters);

void InitSwitches(void);

int main(void) {
    xil_printf("FreeRTOS y switches\r\n");
    InitSwitches();

    switchQueue = xQueueCreate(5, sizeof(u32));
    if (switchQueue == NULL) {
        xil_printf("Error al crear la cola\r\n");
        return -1;
    }
    xTaskCreate(SwitchReaderTask, "SwitchReader", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(SwitchProcessorTask, "SwitchProcessor", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();

    while (1);
    return 0;
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

void SwitchReaderTask(void *pvParameters) {
    u32 currentState;

    while (1) {
        currentState = XGpio_DiscreteRead(&GpioSwitches, 1);

        if (xQueueSend(switchQueue, &currentState, portMAX_DELAY) != pdPASS) {
            xil_printf("Error al encolar el estado de los switches\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void SwitchProcessorTask(void *pvParameters) {
    u32 previousState = 0xFFFFFFFF;
    u32 receivedState;

    while (1) {
        if (xQueueReceive(switchQueue, &receivedState, portMAX_DELAY) == pdPASS) {
            // Procesar solo si el estado cambia
            if (receivedState != previousState) {
                xil_printf("Cambio detectado en los switches: 0x%08X\r\n", receivedState);

                if ((receivedState & 0x01) && !(previousState & 0x01)) {
                    xil_printf("Switch 0 activado\r\n");
                } else if (!(receivedState & 0x01) && (previousState & 0x01)) {
                    xil_printf("Switch 0 desactivado\r\n");
                }
                if ((receivedState & 0x02) && !(previousState & 0x02)) {
                    xil_printf("Switch 1 activado\r\n");
                } else if (!(receivedState & 0x02) && (previousState & 0x02)) {
                    xil_printf("Switch 1 desactivado\r\n");
                }
                if ((receivedState & 0x04) && !(previousState & 0x04)) {
                    xil_printf("Switch 2 activado\r\n");
                } else if (!(receivedState & 0x04) && (previousState & 0x04)) {
                    xil_printf("Switch 2 desactivado\r\n");
                }
                if ((receivedState & 0x08) && !(previousState & 0x08)) {
                    xil_printf("Switch 3 activado\r\n");
                } else if (!(receivedState & 0x08) && (previousState & 0x08)) {
                    xil_printf("Switch 3 desactivado\r\n");
                }
                previousState = receivedState;
            }
        }
    }
}


