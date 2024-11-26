/******************************************************************************/
/*                                                                            */
/* main.c -- Demonstration of a simple use of the Pmod ALS                    */
/*                                                                            */
/******************************************************************************/
/* Author: Thomas Kappenman                                                   */
/* Copyright 2016, Digilent Inc.                                              */
/******************************************************************************/
/* File Description:                                                          */
/*                                                                            */
/* This file contains a basic demo in order to use the Pmod ALS               */
/*                                                                            */
/* Light intensity data is captured and printed over UART.                    */
/* This data can be viewed with a serial terminal application connected to    */
/* your board and configured to use the appropriate Baud rate below           */
/*                                                                            */
/******************************************************************************/
/* Revision History:                                                          */
/*                                                                            */
/*    03/31/2016(tkappenman): Created                                         */
/*    08/24/2017(artvvb):     Validated for Vivado 2015.4                     */
/*    01/23/2018(atangzwj):   Validated for Vivado 2017.4                     */
/*                                                                            */
/******************************************************************************/
/* Baud Rates:                                                                */
/*                                                                            */
/* Microblaze: 9600 or what was specified in UARTlite core                    */
/* Zynq:       115200                                                         */
/*                                                                            */
/******************************************************************************/

#include "PmodALS.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xil_types.h"
#include "xparameters.h"

void DemoInitialize();
void DemoRun();

PmodALS ALS;

int main(void) {
   DemoInitialize();
   DemoRun();
   return 0;
}

void DemoInitialize() {
   ALS_begin(&ALS, XPAR_PMODALS_0_AXI_LITE_SPI_BASEADDR);
}

void DemoRun() {
   u8 light = 0;

   while (1) {
      light = ALS_read(&ALS);
      xil_printf("Light = %d\n\r", light);
      usleep(100000);
   }
}
