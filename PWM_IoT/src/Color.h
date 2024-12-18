#ifndef COLOR_H
#define COLOR_H

#include "PmodCOLOR.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdint.h>

typedef struct {
    uint8_t r;  // Intensidad del color rojo
    uint8_t g;  // Intensidad del color verde
    uint8_t b;  // Intensidad del color azul
} MSGQUEUE_COLOR_t;

extern QueueHandle_t mid_Queue_COLOR;
extern TaskHandle_t xColorTask;

int Init_Color(void);


#endif // COLOR_H
