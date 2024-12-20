/******************************************************************************/
/*                                                                            */
/* main.c -- Example program using the PmodBT2 IP                             */
/*                                                                            */
/******************************************************************************/
/* Author: Arthur Brown                                                       */
/*                                                                            */
/******************************************************************************/
/* File Description:                                                          */
/*                                                                            */
/* This demo continuously polls the Pmod BT2 and host development board's     */
/* UART connections and forwards each character from each to the other.       */
/*                                                                            */
/******************************************************************************/
/* Revision History:                                                          */
/*                                                                            */
/*    10/04/2017(artvvb):   Created                                           */
/*    01/30/2018(atangzwj): Validated for Vivado 2017.4                       */
/*                                                                            */
/******************************************************************************/

#include "Bluetooth.h"
#include "xil_cache.h"
#include "xparameters.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

PmodBT2 myDevice;
SysUart myUart;

void Bluetooth_Initialize();
void SysUartInit();
void ProcessCommand(char *command);

int enabled = 0; // Estado del sistema (0: deshabilitado, 1: habilitado)
int position = 10; // Posici�n inicial del servo (rango 10-90)

int main() {
    u8 buf[64];         // Buffer para el comando
    int n;              // N�mero de bytes recibidos
    int buf_index = 0;  // �ndice del buffer

    Bluetooth_Initialize();

    xil_printf("Sistema Bluetooth inicializado\r\n");
    xil_printf("Esperando comandos...\r\n");

    while (1) {
        // Leer datos del Bluetooth
        n = BT2_RecvData(&myDevice, &buf[buf_index], 1);
        if (n != 0) {
            // Ignorar caracteres de retorno de carro o nueva l�nea
            if (buf[buf_index] == '\r' || buf[buf_index] == '\n') {
                // Procesar comando solo si hay datos en el buffer
                if (buf_index > 0) {
                    buf[buf_index] = '\0'; // Terminar la cadena
                    ProcessCommand((char *)buf);
                    buf_index = 0; // Reiniciar �ndice del buffer
                }
            } else {
                buf_index++;
                // Evitar desbordamiento del buffer
                if (buf_index >= sizeof(buf)) {
                    buf_index = 0;
                }
            }
        }
    }
    return XST_SUCCESS;
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

void SysUartInit() {
#ifdef __MICROBLAZE__
    XUartLite_Initialize(&myUart, SYS_UART_DEVICE_ID);
#else
    XUartPs_Config *myUartCfgPtr;
    myUartCfgPtr = XUartPs_LookupConfig(SYS_UART_DEVICE_ID);
    XUartPs_CfgInitialize(&myUart, myUartCfgPtr, myUartCfgPtr->BaseAddress);
#endif
}

void ProcessCommand(char *command) {
    char response[64];

    if (strcmp(command, "e") == 0) {
        enabled = 1;
        snprintf(response, sizeof(response), "Habilitado\r\n");
    } else if (strcmp(command, "d") == 0) {
        enabled = 0;
        snprintf(response, sizeof(response), "Inhabilitado\r\n");
    } else if (command[0] == '+' && strlen(command) == 1) {
        if (enabled) {
            position += 5;
            if (position > 90) position = 90;
            snprintf(response, sizeof(response), "Posici�n actual = %d\r\n", position);
            response[sizeof(response) - 1] = '\0'; // Garantiza el terminador
        } else {
            snprintf(response, sizeof(response), "Sistema inhabilitado\r\n");
        }
    } else if (command[0] == '-' && strlen(command) == 1) {
        if (enabled) {
            position -= 5;
            if (position < 10) position = 10;
            snprintf(response, sizeof(response), "Posici�n actual = %d\r\n", position);
            response[sizeof(response) - 1] = '\0'; // Garantiza el terminador
        } else {
            snprintf(response, sizeof(response), "Sistema inhabilitado\r\n");
        }
    } else if (strlen(command) == 2 && isdigit(command[0]) && isdigit(command[1])) {
        int new_position = atoi(command);
        if (new_position >= 10 && new_position <= 90) {
            if (enabled) {
                position = new_position;
                snprintf(response, sizeof(response), "Posici�n actual = %d\r\n", position);
                response[sizeof(response) - 1] = '\0'; // Garantiza el terminador
            } else {
                snprintf(response, sizeof(response), "Sistema inhabilitado\r\n");
            }
        } else {
            snprintf(response, sizeof(response), "Mensaje incorrecto\r\n");
        }
    } else {
        snprintf(response, sizeof(response), "Mensaje incorrecto\r\n");
    }

    // Enviar respuesta por Bluetooth
    BT2_SendData(&myDevice, (u8 *)response, strlen(response));

    // Enviar respuesta por UART y usar xil_printf
    xil_printf("%s\r\n", response);
}
