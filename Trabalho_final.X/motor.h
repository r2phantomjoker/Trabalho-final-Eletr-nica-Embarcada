/**
 * @file motor.h
 * @author Arthur Marinho
 * @brief Cabeçalho do driver de motor e sensores (Controle e Odometria).
 * @date Created on November 27, 2025
 */

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/**
 * @brief Callback de baixa frequência para cálculos físicos.
 * Lê o registrador de hardware TMR0 (Encoder), calcula a velocidade
 * e atualiza a posição.
 */
void SENSORES_CalcularVelocidade(void);


void Controle_Subir(void);

void Controle_Descer(void);

void Controle_Parar(void);

void Verificar_Sensores(void);

int Buscar_Proxima_Parada(void);

#endif	/* MOTOR_H */