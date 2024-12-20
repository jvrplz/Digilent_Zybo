/*
 * Principal.h
 *
 *  Created on: 2 dic. 2024
 *      Author: super
 */

#ifndef SRC_PRINCIPAL_H_
#define SRC_PRINCIPAL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define QUEUE_LENGTH 2
#define QUEUE_ITEM_SIZE 32

int Init_Principal(void);
void PrincipalTask(void *pvParameters);
void ProcessCommand(const char *command, char *response, uint8_t *enabled, uint8_t *position);

#endif /* SRC_PRINCIPAL_H_ */
