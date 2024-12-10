#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "PmodCOLOR.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "stdio.h"
#include "stdlib.h"
#include "sleep.h"

#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE sizeof(COLOR_Data)

typedef struct {
   COLOR_Data min, max;
} CalibrationData;

// Colas para comunicación entre tareas
QueueHandle_t colorQueue;

// Prototipos de tareas
void Main_Task(void *pvParameters);
void Color_Task(void *pvParameters);

// Manejo del dispositivo
PmodCOLOR myDevice;

// Tareas
static TaskHandle_t xMainTask;
static TaskHandle_t xColorTask;

// Prototipos auxiliares
void EnableCaches();
void DisableCaches();
void InitializePmodCOLOR();
CalibrationData DemoInitCalibrationData(COLOR_Data firstSample);
void DemoCalibrate(COLOR_Data newSample, CalibrationData *calib);
COLOR_Data DemoNormalizeToCalibration(COLOR_Data sample, CalibrationData calib);

int main(void) {
    EnableCaches();

    // Inicializar el PMOD_COLOR
    InitializePmodCOLOR();
    xil_printf("PmodCOLOR inicializado\r\n");

    // Crear la cola
    colorQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    if (colorQueue == NULL) {
        xil_printf("Error al crear la cola\r\n");
        return -1;
    }

    // Crear las tareas
    xTaskCreate(Color_Task, "Color_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xColorTask);
    xTaskCreate(Main_Task, "Main_Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &xMainTask);

    // Iniciar el scheduler
    vTaskStartScheduler();

    while (1);
    DisableCaches();
    return 0;
}

void InitializePmodCOLOR() {
    COLOR_Begin(&myDevice, XPAR_PMODCOLOR_0_AXI_LITE_IIC_BASEADDR, XPAR_PMODCOLOR_0_AXI_LITE_GPIO_BASEADDR, 0x29);
    COLOR_SetENABLE(&myDevice, COLOR_REG_ENABLE_PON_MASK);
    usleep(2400);
    COLOR_SetENABLE(&myDevice, COLOR_REG_ENABLE_PON_MASK | COLOR_REG_ENABLE_RGBC_INIT_MASK);
    usleep(2400);
}

// Tarea para leer los datos del PMOD_COLOR
void Color_Task(void *pvParameters) {
    COLOR_Data colorData;
    CalibrationData calib;

    // Inicializar la calibración con el primer dato
    colorData = COLOR_GetData(&myDevice);
    calib = DemoInitCalibrationData(colorData);

    while (1) {
        // Leer los datos del sensor
        colorData = COLOR_GetData(&myDevice);

        // Calibrar los valores de color
        DemoCalibrate(colorData, &calib);
        colorData = DemoNormalizeToCalibration(colorData, calib);

        // Enviar los datos a la cola
        if (xQueueSend(colorQueue, &colorData, portMAX_DELAY) != pdPASS) {
            xil_printf("Error al enviar datos a la cola\r\n");
        }

        // Retardo para la próxima lectura
        vTaskDelay(pdMS_TO_TICKS(100)); // 100 ms
    }
}

// Tarea principal para procesar y mostrar los datos
void Main_Task(void *pvParameters) {
    COLOR_Data receivedData;

    while (1) {
        // Recibir los datos desde la cola
        if (xQueueReceive(colorQueue, &receivedData, portMAX_DELAY) == pdPASS) {
            // Convertir los valores a rango 0-255
            u8 red = (receivedData.r * 255) / 0xFFFF;
            u8 green = (receivedData.g * 255) / 0xFFFF;
            u8 blue = (receivedData.b * 255) / 0xFFFF;
            u8 brightness = (receivedData.c * 255) / 0xFFFF;

            // Mostrar los valores RGB y brillo
            xil_printf("R=%d G=%d B=%d\n\r", red, green, blue);
            xil_printf("Brillo= %d\n\r", brightness);
        }
    }
}

CalibrationData DemoInitCalibrationData(COLOR_Data firstSample) {
    CalibrationData calib;
    calib.min = firstSample;
    calib.max = firstSample;
    return calib;
}

void DemoCalibrate(COLOR_Data newSample, CalibrationData *calib) {
    if (newSample.c < calib->min.c) calib->min.c = newSample.c;
    if (newSample.r < calib->min.r) calib->min.r = newSample.r;
    if (newSample.g < calib->min.g) calib->min.g = newSample.g;
    if (newSample.b < calib->min.b) calib->min.b = newSample.b;

    if (newSample.c > calib->max.c) calib->max.c = newSample.c;
    if (newSample.r > calib->max.r) calib->max.r = newSample.r;
    if (newSample.g > calib->max.g) calib->max.g = newSample.g;
    if (newSample.b > calib->max.b) calib->max.b = newSample.b;
}

COLOR_Data DemoNormalizeToCalibration(COLOR_Data sample, CalibrationData calib) {
    COLOR_Data norm;
    norm.c = (sample.c - calib.min.c) * (0xFFFF / (calib.max.c - calib.min.c));
    norm.r = (sample.r - calib.min.r) * (0xFFFF / (calib.max.r - calib.min.r));
    norm.g = (sample.g - calib.min.g) * (0xFFFF / (calib.max.g - calib.min.g));
    norm.b = (sample.b - calib.min.b) * (0xFFFF / (calib.max.b - calib.min.b));
    return norm;
}

void EnableCaches() {
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_ICACHE
    Xil_ICacheEnable();
#endif
#ifdef XPAR_MICROBLAZE_USE_DCACHE
    Xil_DCacheEnable();
#endif
#endif
}

void DisableCaches() {
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_DCACHE
    Xil_DCacheDisable();
#endif
#ifdef XPAR_MICROBLAZE_USE_ICACHE
    Xil_ICacheDisable();
#endif
#endif
}
