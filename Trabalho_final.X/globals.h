/* 
 * File:   globals.h
 * Author: Arthur Marinho
 *
 * Created on November 27, 2025, 11:17 AM
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdbool.h>

// Definição dos Estados do Motor (para facilitar a leitura)
#define MOTOR_PARADO    0
#define MOTOR_SUBINDO   1
#define MOTOR_DESCENDO  2


// Variáveis exigidas na Tabela 1 do Roteiro (Telemetria)
extern volatile uint8_t andar_atual;      // 0 a 3
extern volatile uint8_t andar_destino;    // 0 a 3
extern volatile uint8_t estado_motor;     // 0=Parado, 1=Subindo, 2=Descendo
extern volatile uint8_t posicao_mm;       // 0 a 180 mm
extern volatile float velocidade_atual;   // em mm/s
extern volatile float temperatura_ponte;  // em Graus Celsius

// Variáveis de Lógica Interna
extern volatile bool solicitacoes[4];     // Vetor: [0]=Térreo, [1]=1ºAndar... (true/false)

#endif