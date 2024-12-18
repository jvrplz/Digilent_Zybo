/*------------------------------------------------------------------------------
 *      Módulo COLOR: Tarea que se encarga de la lectura periódica de los datos de color
 *      proporcionados por el sensor PmodCOLOR. Una vez realizada la lectura,
 *      encola sus valores al módulo principal
 *-----------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "color.h"
#include "PmodCOLOR.h"
#include "sleep.h"
#include "xil_printf.h"
#include "xparameters.h"

#define QUEUE_LENGTH 1
#define QUEUE_ITEM_SIZE 16

#define COLOR_RegATIME  0x01
#define COLOR_RegCONTROL 0x0F

#define ATIME_600MS 0x06
#define GAIN_60X 0x03

PmodCOLOR myColorDevice;
TaskHandle_t xColorTask;

void ColorTask(void *pvParameters);

QueueHandle_t mid_Queue_COLOR;

MSGQUEUE_COLOR_t color;

int Init_Color(void) {
	mid_Queue_COLOR = xQueueCreate(QUEUE_LENGTH, sizeof(MSGQUEUE_COLOR_t));
    if (mid_Queue_COLOR == NULL) {
        xil_printf("Error al crear la cola de COLOR\r\n");
        return -1;
    }

    BaseType_t tid_color = xTaskCreate(ColorTask, "Color_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, &xColorTask);
    if (tid_color != pdPASS) {
        xil_printf("Error al crear la tarea Color_Task\r\n");
        return -1;
    }

    return 0;
}

static void Color_Initialize() {
    COLOR_Begin(&myColorDevice, XPAR_PMODCOLOR_0_AXI_LITE_IIC_BASEADDR, XPAR_PMODCOLOR_0_AXI_LITE_GPIO_BASEADDR, 0x29);

    COLOR_SetLED(&myColorDevice, 0);

    COLOR_SetENABLE(&myColorDevice, 0x03);

    // Tiempo de integración
    {
        u8 atime = ATIME_600MS;
        COLOR_WriteIIC(&myColorDevice, COLOR_RegATIME, &atime, 1);
    }

    // Ganancia 60x
    {
        u8 control = GAIN_60X;
        COLOR_WriteIIC(&myColorDevice, COLOR_RegCONTROL, &control, 1);
    }

    vTaskDelay(pdMS_TO_TICKS(700));
}

void ColorTask(void *pvParameters) {
    Color_Initialize();
    COLOR_Data data = COLOR_GetData(&myColorDevice);
    COLOR_SetLED(&myColorDevice, 0);

    static uint8_t prevMajorityColor = 0xFF; // 0xFF indica que no hay color previo aún

    while (1) {
        // Esperar notificación antes de medir
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        COLOR_SetLED(&myColorDevice, 1);
        // Esperar el tiempo de integración (~700 ms)
        vTaskDelay(pdMS_TO_TICKS(700));

        data = COLOR_GetData(&myColorDevice);
        COLOR_SetLED(&myColorDevice, 0);

        color.r = (data.r * 255) / 65535; // Convertir a [0,255]
        color.g = (data.g * 255) / 65535;
        color.b = (data.b * 255) / 65535;

        // Determinar color mayoritario
        char majorityColor = 0; // 'R', 'G', 'B', o 0 si no hay mayoritario claro
        if (color.r > color.g && color.r > color.b) {
            majorityColor = 'R';
        } else if (color.g > color.r && color.g > color.b) {
            majorityColor = 'G';
        } else if (color.b > color.r && color.b > color.g) {
            majorityColor = 'B';
        }

        // Si hay un mayoritario claro y es distinto al anterior
        if (majorityColor != 0 && majorityColor != prevMajorityColor) {
            // Enviar a la cola solo si cambió el mayoritario
            if (xQueueSend(mid_Queue_COLOR, &color, portMAX_DELAY) != pdPASS) {
                xil_printf("Error al enviar color a la cola\r\n");
                xQueueReset(mid_Queue_COLOR);
            }
            prevMajorityColor = (uint8_t)majorityColor;
        }

        // Si no cambió el mayoritario o no hubo mayoritario claro, no se envía nada.
        // La tarea quedará esperando la próxima notificación para volver a medir.
        taskYIELD();
    }
}
