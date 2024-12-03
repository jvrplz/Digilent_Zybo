#include "Principal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "xparameters.h"
#include "Bluetooth.h"
#include "ALS.h"

BaseType_t tid_Principal;

typedef enum {
    MODO1,
	MODO2,
	MODO3
} Estado;

char almacen[QUEUE_ITEM_SIZE];
int Init_Principal(){

	tid_Principal = xTaskCreate(PrincipalTask, "Principal", 512, NULL, tskIDLE_PRIORITY, NULL);
	if(tid_Principal != pdPASS){
		return -1;
	}
	Init_Bluetooth();
	Init_ALS();
	//Resto de Inits

	return 0;
}

void PrincipalTask(void *pvParameters) {
	MSGQUEUE_BLU_TX_t bluetx;
	MSGQUEUE_BLU_RX_t bluerx;
	MSGQUEUE_ALS_t als;

	Estado state = MODO1;

    int position = 10;
    int enabled = 0;

    while (1) {
    	switch (state) {
    	    case MODO1://BLUETOOTH
    	    	if (xQueueReceive(mid_Queue_RX_Blue, &bluerx, portMAX_DELAY) == pdTRUE) {
    	    		ProcessCommand(bluerx.string, bluetx.string, &enabled, &position);
    	    		xQueueSend(mid_Queue_TX_Blue, &bluetx, portMAX_DELAY);
    	    	}
    	        break;

    	    case MODO2://MEDIDA PERIODICA
    	    	xTaskNotifyGive(xLightTask);
    	    	if (xQueueReceive(mid_Queue_ALS, &als, portMAX_DELAY) == pdTRUE) {
    	    		//bluetx.string = als.light;//REVISAR ESTE MENSAJE porque no se hace así
    	    		xQueueSend(mid_Queue_TX_Blue, &bluetx, portMAX_DELAY);
    	    	}
    	        break;

    	    case MODO3://seria COM-PC
    	    	if (xQueueReceive(mid_Queue_RX_Blue, &bluerx, portMAX_DELAY) == pdTRUE) {
    	    		ProcessCommand(bluerx.string, bluetx.string, &enabled, &position);
    	    		xQueueSend(mid_Queue_TX_Blue, &bluetx, portMAX_DELAY);
    	    	}
    	        break;

    	    default:
    	        break;
    	}
    	taskYIELD();
    }
}

void ProcessCommand(const char *command, char *response, int *enabled, int *position) {
    size_t len = strlen(command);

    if (len == 1) {
        char cmd = command[0];

        switch (cmd) {
            case 'e':
                *enabled = 1;
                snprintf(response, QUEUE_ITEM_SIZE, "Habilitado\r\n");
                break;

            case 'd':
                *enabled = 0;
                snprintf(response, QUEUE_ITEM_SIZE, "Inhabilitado\r\n");
                break;

            case '+':
            case '-':
                if (*enabled) {
                    int delta = (cmd == '+') ? 5 : -5;
                    *position += delta;

                    // Asegurar que la posición esté dentro del rango [10, 90]
                    if (*position > 90) *position = 90;
                    if (*position < 10) *position = 10;

                    snprintf(response, QUEUE_ITEM_SIZE, "Posición actual = %d\r\n", *position);
                } else {
                    snprintf(response, QUEUE_ITEM_SIZE, "Sistema inhabilitado\r\n");
                }
                break;

            default:
                snprintf(response, QUEUE_ITEM_SIZE, "Mensaje incorrecto\r\n");
                break;
        }

    } else if (len == 2 && isdigit((unsigned char)command[0]) && isdigit((unsigned char)command[1])) {
        int new_position = atoi(command);

        if (new_position >= 10 && new_position <= 90) {
            if (*enabled) {
                *position = new_position;
                snprintf(response, QUEUE_ITEM_SIZE, "Posición actual = %d\r\n", *position);
            } else {
                snprintf(response, QUEUE_ITEM_SIZE, "Sistema inhabilitado\r\n");
            }
        } else {
            snprintf(response, QUEUE_ITEM_SIZE, "Mensaje incorrecto\r\n");
        }

    } else {
        snprintf(response, QUEUE_ITEM_SIZE, "Mensaje incorrecto\r\n");
    }
}
