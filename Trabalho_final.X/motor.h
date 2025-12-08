/**
 * @file motor.h
 * @brief Cabeçalho do driver de motor e sensores.
 * @details Este arquivo contém as declarações das funções de controle de
 * movimentação, leitura de sensores e lógica de escalonamento do elevador.
 */

#ifndef MOTOR_H
#define MOTOR_H


#include <stdint.h>
#include <stdbool.h>

// ================================
// FUNÇÕES DE TELEMETRIA E SENSORES
// ================================

/**
 * @brief Atualiza a telemetria do sistema (Velocidade e Posição).
 * @details Esta função deve ser chamada periodicamente para garantir
 * a precisão do cálculo da velocidade do encoder.
 */
void SENSORES_CalcularVelocidade(void);

/**
 * @brief Lê os sensores de andar.
 * @details Atualiza a variável global de "andar atual" e verifica colisões
 * (segurança de hardware) nos extremos do elevador.
 */
void Verificar_Sensores(void);

// ================================
// FUNÇÕES DE CONTROLE DE MOVIMENTO
// ================================

/**
 * @brief Ativa o motor para subir o elevador.
 * @note Configura o PWM e a direção automaticamente.
 */
void Controle_Subir(void);

/**
 * @brief Ativa o motor para descer o elevador.
 * @note Configura o PWM e a direção automaticamente.
 */
void Controle_Descer(void);

/**
 * @brief Para o motor imediatamente.
 * @details Desliga o PWM e coloca o estado do motor em repouso.
 */
void Controle_Parar(void);

// ====================
// ALGORITMOS DE LÓGICA
// ====================

/**
 * @brief Verifica requisições acima de um andar.
 * @param andar_ref Andar de referência.
 * @return true/false.
 */
bool Existe_Chamada_Acima(uint8_t andar_ref);

/**
 * @brief Verifica requisições abaixo de um andar.
 * @param andar_ref Andar de referência.
 * @return true/false.
 */
bool Existe_Chamada_Abaixo(uint8_t andar_ref);

/**
 * @brief Remove a pendência do andar atual dos vetores globais.
 */
void Limpar_Chamada_Atual(void);  

#endif	/* MOTOR_H */