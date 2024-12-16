#ifndef SRC_SW_H_
#define SRC_SW_H_

#define SW0 0x01
#define SW3 0x08

typedef struct {
	uint16_t value;
} MSGQUEUE_SW_t;

extern QueueHandle_t mid_Queue_sw;

int Init_sw(void);
void InitSwitches(void);



#endif /* SRC_SW_H_ */
