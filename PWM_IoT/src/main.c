#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "Principal.h"

int main(void) {
	Init_Principal();
	vTaskStartScheduler();

    while (1);
    return 0;
}
