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
 * @brief Para o motor imediatamente e atualiza o estado global.
 * Zera o PWM e define o estado como MOTOR_PARADO.
 */
void MOTOR_parar(void);

/**
 * @brief Realiza a calibração inicial.
 * Bloqueia o sistema, forçando a descida até encontrar o sensor S1 (Térreo).
 * Zera todas as variáveis de posição e o hardware do Timer 0.
 */
void MOTOR_reset(void);

/**
 * @brief Move o elevador até o andar de destino.
 * Função bloqueante que gerencia o movimento passo-a-passo.
 * O Timer 0 continua contando pulsos automaticamente durante a execução.
 * @param destino Andar alvo (0-3).
 * @param atual Andar atual (0-3).
 */
void MOTOR_mover(uint8_t destino, uint8_t atual);


/**
 * @brief Callback de baixa frequência para cálculos físicos.
 * Lê o registrador de hardware TMR0 (Encoder), calcula a velocidade
 * e atualiza a posição.
 */
void SENSORES_CalcularVelocidade(void);

#endif	/* MOTOR_H */