/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include <xparameters.h>
#include <xgpio.h>

/* Definitions for peripheral AXI_GPIO_0 */
#define SWITCHES_DEVICE_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define XPAR_AXI_GPIO_0_BASEADDR 0x41200000
#define XPAR_AXI_GPIO_0_HIGHADDR 0x4120FFFF
#define XPAR_AXI_GPIO_0_INTERRUPT_PRESENT 0
#define XPAR_AXI_GPIO_0_IS_DUAL 0

XGpio GpioSwitches; // Instancia del GPIO para los switches

void InitSwitches(void);
void CheckSwitches(void);

u32 previousSwitchStatus = 0xFFFFFFFF; //fuera del rango válido

int main(void) {
    xil_printf("Hola mundo switches\r\n");
    InitSwitches();

    while (1) {
        CheckSwitches();
        for (volatile int i = 0; i < 1000000; i++);
    }

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

void CheckSwitches(void) {
    u32 currentSwitchStatus;
    currentSwitchStatus = XGpio_DiscreteRead(&GpioSwitches, 1);

    if (currentSwitchStatus != previousSwitchStatus) {
        xil_printf("Cambio detectado en los switches: 0x%08X\r\n", currentSwitchStatus);

        if (currentSwitchStatus & 0x01) {
            xil_printf("Switch 1 activado\r\n");
        } else if ((previousSwitchStatus & 0x01) && !(currentSwitchStatus & 0x01)) {
            xil_printf("Switch 1 desactivado\r\n");
        }
        if (currentSwitchStatus & 0x02) {
            xil_printf("Switch 2 activado\r\n");
        } else if ((previousSwitchStatus & 0x02) && !(currentSwitchStatus & 0x02)) {
            xil_printf("Switch 2 desactivado\r\n");
        }
        if (currentSwitchStatus & 0x04) {
            xil_printf("Switch 3 activado\r\n");
        } else if ((previousSwitchStatus & 0x04) && !(currentSwitchStatus & 0x04)) {
            xil_printf("Switch 3 desactivado\r\n");
        }
        if (currentSwitchStatus & 0x08) {
            xil_printf("Switch 4 activado\r\n");
        } else if ((previousSwitchStatus & 0x08) && !(currentSwitchStatus & 0x08)) {
            xil_printf("Switch 4 desactivado\r\n");
        }
        previousSwitchStatus = currentSwitchStatus;
    }
}
