#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Principal.h"


int main(void) {
    // Crear tareas
    xTaskCreate(PrincipalTask, "Principal", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &xMainTask);

    // Iniciar el scheduler
    vTaskStartScheduler();

    while (1); // Nunca debería llegar aquí
    return 0;
}
