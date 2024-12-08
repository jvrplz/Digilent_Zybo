#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ALS.h"

XSpi_Config XSpi_ALSConfig ={0, 0, 1, 0, 1, 8, 0, 0, 0, 0, 0};

PmodALS ALS;
TaskHandle_t xLightTask;

void ALS_Task(void *pvParameters);

BaseType_t tid_ALS;

QueueHandle_t mid_Queue_ALS;

MSGQUEUE_ALS_t als;

int Init_ALS(void){
	mid_Queue_ALS = xQueueCreate(QUEUE_LENGTH, sizeof(MSGQUEUE_ALS_t));

    if (mid_Queue_ALS == NULL) {
        xil_printf("Error al crear la cola ALS\r\n");
        return -1;
    }

    tid_ALS = xTaskCreate(ALS_Task, "ALS_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &xLightTask);
	if(tid_ALS != pdPASS){
		return -1;
	}
    return 0;
}

void ALS_Task(void *pvParameters) {

	ALS_begin(&ALS, XPAR_PMODALS_0_AXI_LITE_SPI_BASEADDR);
	vTaskDelay(pdMS_TO_TICKS(100));

    while (1) {
    	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        als.light = ALS_read(&ALS);
        if (xQueueSend(mid_Queue_ALS, &als, portMAX_DELAY) != pdPASS) {
            xil_printf("Error al enviar el valor a la cola\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

XStatus ALS_begin(PmodALS *InstancePtr, u32 SPI_Address) {
   XSpi_ALSConfig.BaseAddress = SPI_Address;
   return ALS_SPIInit(&InstancePtr->ALSSpi);
}

XStatus ALS_SPIInit(XSpi *SpiInstancePtr) {
   XStatus Status;
   XSpi_Config *ConfigPtr;
   /*
    * Initialize the SPI driver so that it is ready to use.
    */
   ConfigPtr = &XSpi_ALSConfig;
   if (ConfigPtr == NULL) {
      return XST_DEVICE_NOT_FOUND;
   }

   Status = XSpi_CfgInitialize(SpiInstancePtr, ConfigPtr,
         ConfigPtr->BaseAddress);
   if (Status != XST_SUCCESS) {
      return XST_FAILURE;
   }

   u32 options = (XSP_MASTER_OPTION | XSP_CLK_ACTIVE_LOW_OPTION
         | XSP_CLK_PHASE_1_OPTION) | XSP_MANUAL_SSELECT_OPTION;
   Status =
         XSpi_SetOptions(SpiInstancePtr, options); // Manual SS off
   if (Status != XST_SUCCESS) {
      return XST_FAILURE;
   }

   Status = XSpi_SetSlaveSelect(SpiInstancePtr, 1);
   if (Status != XST_SUCCESS) {
      return XST_FAILURE;
   }
   /*
    * Start the SPI driver so that the device is enabled.
    */
   XSpi_Start(SpiInstancePtr);
   /*
    * Disable Global interrupt to use polled mode operation
    */
   XSpi_IntrGlobalDisable(SpiInstancePtr);

   return XST_SUCCESS;
}

u8 ALS_read(PmodALS* InstancePtr) {
   u8 light[2];
   XSpi_Transfer(&InstancePtr->ALSSpi, light, light, 2);
   return (light[0] << 3) | (light[1] >> 5);
}
