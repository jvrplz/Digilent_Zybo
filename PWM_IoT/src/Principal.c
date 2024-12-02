#include "principal.h"
#include "FreeRTOS.h"
#include "task.h"

// Prototipos de tareas

TaskHandle_t xMainTask;

// Implementaci�n de la tarea principal
void PrincipalTask(void *pvParameters) {
    // Crear la tarea Bluetooth
    //xTaskCreate(Bluetooth_ReceiveTask, "BT_Receive", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

    // Aqu� puedes gestionar la l�gica principal del sistema
    while (1) {
        // Realizar tareas de gesti�n principal
        vTaskDelay(pdMS_TO_TICKS(1000)); // Retardo para evitar bucles apretados
    }
}
