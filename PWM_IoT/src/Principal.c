/*------------------------------------------------------------------------------
 *      Módulo Principal: Tarea que se encarga de coordinar todas las demás.
 *      Es el módulo que decide las acciones a tomar en función del estado del
 *		sistema y de la información que reciba del resto de los módulos
 *-----------------------------------------------------------------------------*/

#include "Principal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "xparameters.h"
#include "Bluetooth.h"
#include "ALS.h"
#include "sw.h"

BaseType_t tid_Principal;

typedef enum {
    MODO1,
	MODO2,
	MODO3,
	COLOR
} Estado;

char almacen[QUEUE_ITEM_SIZE];
int Init_Principal(){

	tid_Principal = xTaskCreate(PrincipalTask, "Principal", 512, NULL, tskIDLE_PRIORITY, NULL);
	if(tid_Principal != pdPASS){
		return -1;
	}
	Init_Bluetooth();
	Init_ALS();
	Init_sw();
	//Resto de Inits

	return 0;
}

void PrincipalTask(void *pvParameters) {
	MSGQUEUE_BLU_TX_t bluetx;
	MSGQUEUE_BLU_RX_t bluerx;
	MSGQUEUE_PC_TX_t pctx;
	MSGQUEUE_PC_RX_t pcrx;
	MSGQUEUE_ALS_t als;
	MSGQUEUE_SW_t sw;

	Estado state = MODO1;

    uint8_t position = 50;
    uint8_t enabled = 0;
    uint16_t aux_sw = 0;
    //Dormir tareas innecesarias

    while (1) {

    	if (xQueueReceive(mid_Queue_sw, &sw, 0) == pdTRUE) {
    		aux_sw = sw.value;
    		xil_printf("%d\r\n", aux_sw);
    	}

    	switch (state) {
    	    case MODO1://BLUETOOTH
    	    	if (xQueueReceive(mid_Queue_RX_Blue, &bluerx, 0) == pdTRUE) {
    	    		ProcessCommand(bluerx.string, bluetx.string, &enabled, &position);
    	    		xQueueSend(mid_Queue_TX_Blue, &bluetx, portMAX_DELAY);
    	    		if(enabled){
    	    			xil_printf("%d\r\n", position);
    	    		}
    	    	}
    	    	//setear pwm

    	    	if (aux_sw == SW0){
    	    		//state = COLOR;
    	    	}
    	    	if (aux_sw == SW3){
    	    		state = MODO3;
    	    	}
    	        break;

    	    case MODO2://MEDIDA PERIODICA
    	    	xTaskNotifyGive(xLightTask);//MOVER DE LADO
    	    	if (xQueueReceive(mid_Queue_ALS, &als, 0) == pdTRUE) {
    	    		snprintf(bluetx.string, sizeof(bluetx.string), "%u\r\n", als.light);
    	    		xQueueSend(mid_Queue_TX_Blue, &bluetx, portMAX_DELAY);
    	    	}
    	    	//Setear pwm
    	    	if (aux_sw == SW0){
    	    		state = COLOR;
    	    	}
    	    	if (aux_sw == SW3){
    	    		state = MODO3;
    	    	}
    	        break;

    	    case MODO3://COM-PC
    	    	if (xQueueReceive(mid_Queue_RX_Pc, &pcrx, 0) == pdTRUE) {
    	    		ProcessCommand(pcrx.string, pctx.string, &enabled, &position);
    	    		xQueueSend(mid_Queue_TX_Pc, &pctx, portMAX_DELAY);
    	    	}
    	    	if (aux_sw == SW0){
    	    		state = COLOR;
    	    	}
    	    	//setear pwm
    	        break;
    	    case COLOR:
    	    	//Lógica
    	    	// if recieve, en función del rgb obtenido cambia de modo
    	    	//por ejemplo R1 G2 B3
    	    	break;

    	    default:
    	        break;
    	}
    }
}

void ProcessCommand(const char *command, char *response, uint8_t *enabled, uint8_t *position) {
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
                    if (*position > 90) *position = 90;
                    if (*position < 10) *position = 10;

                    snprintf(response, QUEUE_ITEM_SIZE, "Posicion actual = %d\r\n", *position);
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
                snprintf(response, QUEUE_ITEM_SIZE, "Posicion actual = %d\r\n", *position);
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
