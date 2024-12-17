/*------------------------------------------------------------------------------
 *      COLOR.h: Archivo de cabecera para el módulo PmodCOLOR
 *-----------------------------------------------------------------------------*/

#ifndef COLOR_H
#define COLOR_H

#include "PmodCOLOR.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Definiciones */
#define QUEUE_LENGTH 2
#define QUEUE_ITEM_SIZE sizeof(COLOR_Data)

/* Estructuras */
typedef struct {
   COLOR_Data min;
   COLOR_Data max;
} CalibrationData;

typedef struct {
    uint8_t r;  // Intensidad del color rojo
    uint8_t g;  // Intensidad del color verde
    uint8_t b;  // Intensidad del color azul
} MSGQUEUE_COLOR_t;


/* Variables Globales */
extern TaskHandle_t xColorTask;
extern QueueHandle_t mid_Queue_COLOR;
extern CalibrationData calib;
extern COLOR_Data colorData;

/* Prototipos de funciones */
int Init_Color(void);
CalibrationData COLOR_InitCalibrationData(COLOR_Data firstSample);
void COLOR_Calibrate(COLOR_Data newSample, CalibrationData *calib);
COLOR_Data COLOR_NormalizeToCalibration(COLOR_Data sample, CalibrationData calib);
void COLORTask(void *pvParameters);

#endif /* COLOR_H */
