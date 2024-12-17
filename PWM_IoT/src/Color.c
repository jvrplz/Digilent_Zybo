/*------------------------------------------------------------------------------
 *      Módulo COLOR: Tarea que se encarga de la lectura periódica de los datos de color
 *      proporcionados por el sensor PmodCOLOR. Una vez realizada la lectura,
 *      encola sus valores al módulo principal
 *-----------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "PmodCOLOR.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xparameters.h"

#define QUEUE_LENGTH 2
#define QUEUE_ITEM_SIZE sizeof(COLOR_Data)

/* Configuración e instancias */
PmodCOLOR COLOR;
TaskHandle_t xColorTask;
QueueHandle_t mid_Queue_COLOR;

void COLORTask(void *pvParameters);

typedef struct {
   COLOR_Data min, max;
} CalibrationData;

/* Variables globales */
BaseType_t tid_COLOR;
CalibrationData calib;
COLOR_Data colorData;

/* Prototipos de funciones */
int Init_Color(void);
CalibrationData COLOR_InitCalibrationData(COLOR_Data firstSample);
void COLOR_Calibrate(COLOR_Data newSample, CalibrationData *calib);
COLOR_Data COLOR_NormalizeToCalibration(COLOR_Data sample, CalibrationData calib);

/* Inicialización del módulo COLOR */
int Init_Color(void) {
    mid_Queue_COLOR = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    if (mid_Queue_COLOR == NULL) {
        xil_printf("Error al crear la cola COLOR\r\n");
        return -1;
    }

    tid_COLOR = xTaskCreate(COLORTask, "COLOR_Task", 256, NULL, tskIDLE_PRIORITY + 2, &xColorTask);
    if (tid_COLOR != pdPASS) {
        xil_printf("Error al crear la tarea COLOR\r\n");
        return -1;
    }
    return 0;
}

/* Tarea principal del módulo COLOR */
void COLORTask(void *pvParameters) {
    COLOR_Begin(&COLOR, XPAR_PMODCOLOR_0_AXI_LITE_IIC_BASEADDR, XPAR_PMODCOLOR_0_AXI_LITE_GPIO_BASEADDR, 0x29);
    COLOR_SetENABLE(&COLOR, COLOR_REG_ENABLE_PON_MASK);
    usleep(2400);
    COLOR_SetENABLE(&COLOR, COLOR_REG_ENABLE_PON_MASK | COLOR_REG_ENABLE_RGBC_INIT_MASK);
    usleep(2400);

    colorData = COLOR_GetData(&COLOR);
    calib = COLOR_InitCalibrationData(colorData);

    while (1) {
        colorData = COLOR_GetData(&COLOR);
        COLOR_Calibrate(colorData, &calib);
        colorData = COLOR_NormalizeToCalibration(colorData, calib);

        if (xQueueSend(mid_Queue_COLOR, &colorData, portMAX_DELAY) != pdPASS) {
            xil_printf("Error al enviar el valor a la cola COLOR\r\n");
        }

        xil_printf("R=%03d G=%03d B=%03d\r\n", colorData.r, colorData.g, colorData.b);
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

/* Funciones de calibración */
CalibrationData COLOR_InitCalibrationData(COLOR_Data firstSample) {
    CalibrationData calib;
    calib.min = firstSample;
    calib.max = firstSample;
    return calib;
}

void COLOR_Calibrate(COLOR_Data newSample, CalibrationData *calib) {
    if (newSample.c < calib->min.c) calib->min.c = newSample.c;
    if (newSample.r < calib->min.r) calib->min.r = newSample.r;
    if (newSample.g < calib->min.g) calib->min.g = newSample.g;
    if (newSample.b < calib->min.b) calib->min.b = newSample.b;

    if (newSample.c > calib->max.c) calib->max.c = newSample.c;
    if (newSample.r > calib->max.r) calib->max.r = newSample.r;
    if (newSample.g > calib->max.g) calib->max.g = newSample.g;
    if (newSample.b > calib->max.b) calib->max.b = newSample.b;
}

COLOR_Data COLOR_NormalizeToCalibration(COLOR_Data sample, CalibrationData calib) {
    COLOR_Data norm;
    norm.c = (sample.c - calib.min.c) * (255.0 / (calib.max.c - calib.min.c));
    norm.r = (sample.r - calib.min.r) * (255.0 / (calib.max.r - calib.min.r));
    norm.g = (sample.g - calib.min.g) * (255.0 / (calib.max.g - calib.min.g));
    norm.b = (sample.b - calib.min.b) * (255.0 / (calib.max.b - calib.min.b));
    return norm;
}
