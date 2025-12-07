/**
 * @file globals.h
 * @author Arthur Marinho
 * @date Created on November 27, 2025, 11:17 AM
 * @brief Definições globais, constantes de hardware e variáveis de estado do Elevador.
 *
 * Este arquivo centraliza as variáveis exigidas para a telemetria (Tabela 1 do Roteiro)
 * e permite o compartilhamento de dados entre os módulos de Motor, Sensores e UART.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <xc.h>         // Necessário para reconhecer LATAbits
#include <stdint.h>     // Necessário para uint8_t, int16_t, etc.
#include <stdbool.h>    // Necessário para o tipo bool

// ==========================================
// MÁQUINA DE ESTADOS E CONTROLE
// ==========================================

/** * @brief Estados possíveis do motor (Variável 'M' na Tabela 1). 
 */
#define MOTOR_PARADO    0   ///< Motor desligado e travado
#define MOTOR_SUBINDO   1   ///< Movimento ascendente (Direção = 1)
#define MOTOR_DESCENDO  2   ///< Movimento descendente (Direção = 0)


// Define o pino como nível ALTO (5V/3.3V)
#define CS_SetHigh()    LATBbits.LATB1 = 1

// Define o pino como nível BAIXO (0V/GND)
#define CS_SetLow()     LATBbits.LATB1 = 0

// ==========================================
// MAPEAMENTO DE HARDWARE (Sensores de Andar)
// ==========================================

/**
 * @brief Sensores Digitais (Térreo e 1º Andar).
 * Conectados ao PORTB com Pull-Up ativado.
 * Tipo: Sensor de Efeito Hall (A3144) - Coletor Aberto.
 * * Lógica:
 * - 1 (HIGH): Sem ímã (Elevador longe).
 * - 0 (LOW):  Com ímã (Elevador no andar).
 */
#define SENSOR_S1       PORTBbits.RB0   ///< Sensor do Térreo (Andar 0). Leitura direta do pino.
#define SENSOR_S2       PORTBbits.RB3   ///< Sensor do 1º Andar (Andar 1). Leitura direta do pino.

/**
 * @brief Sensores Analógicos (2º e 3º Andar).
 * Conectados ao PORTA e lidos através dos Comparadores Analógicos.
 * Configuração: FVR (2.048V) na entrada Positiva, Sensor na Negativa.
 * * Lógica (Invertida pelo Comparador):
 * - 0 (LOW):  Sem ímã (Sensor=5V > Ref=2V -> Saída 0).
 * - 1 (HIGH): Com ímã (Sensor=0V < Ref=2V -> Saída 1).
 */
#define SENSOR_S3       CM1CON0bits.C1OUT  ///< Sensor do 2º Andar (Andar 2). Lê a saída do Comparador 1.
#define SENSOR_S4       CM2CON0bits.C2OUT  ///< Sensor do 3º Andar (Andar 3). Lê a saída do Comparador 2.


// ==========================================
// MAPEAMENTO DE HARDWARE (Camada HAL)
// ==========================================

/** * @brief Controle do Pino de Direção (RA7).
 * Usa o registrador LAT para escrita segura na saída.
 */
#define DIR             LATAbits.LATA7 
#define DIRECAO_SUBIR   1   ///< Define pino RA7 em Nível Alto (Subida)
#define DIRECAO_DESCER  0   ///< Define pino RA7 em Nível Baixo (Descida)

/**
 * @brief Valores de PWM (0 a 1023 - 10 bits).
 * O roteiro sugere ciclo útil entre 40% e 50%[cite: 388].
 * 410 = ~40% de Duty Cycle.
 */
#define MOTOR_OFF       0
#define MOTOR_ON        614 

// ==========================================
// VARIÁVEIS GLOBAIS (TELEMETRIA - TABELA 1)
// ==========================================

/**
 * @brief 'Andar (A)': Último andar detectado pelos sensores.
 * Faixa: 0 a 3.
 */
extern volatile uint8_t andar_atual;

/**
 * @brief 'Destino (D)': Andar alvo da solicitação atual.
 * Faixa: 0 a 3.
 */
extern volatile uint8_t andar_destino;

/**
 * @brief 'Motor (M)': Estado atual do movimento.
 * Valores: 0 (Parado), 1 (Subindo), 2 (Descendo).
 */
extern volatile uint8_t estado_motor;

/**
 * @brief 'Posição (HHH)': Altura em milímetros em relação ao solo.
 * Faixa: 0 a 180 mm[cite: 385].
 */
extern volatile uint8_t posicao_mm;

/**
 * @brief 'Velocidade (VV.V)': Velocidade instantânea calculada via Encoder.
 * Unidade: mm/s.
 */
extern volatile uint8_t velocidade_atual;

/**
 * @brief 'Temperatura (TT.T)': Temperatura monitorada na Ponte H.
 * Unidade: Graus Celsius (°C).
 */
extern volatile uint16_t temperatura_ponte;


// ==========================================
// VARIÁVEIS DE LÓGICA INTERNA
// ==========================================

/**
 * @brief Fila de solicitações de chamadas.
 * Índice [0] = Térreo ... [3] = 3º Andar.
 * true = Botão pressionado / Solicitação ativa.
 */
extern volatile bool solicitacoes[4];

typedef enum {
    ESTADO_PARADO,
    ESTADO_SUBINDO,
    ESTADO_DESCENDO,
    ESTADO_ESPERA_PORTA,
    ESTADO_REVERSAO
} EstadoElevador;
//Se alterar a ordem desse enum, alterar a ordem da LUT_dir em comm.h

extern volatile EstadoElevador estado_atual;

/**
 * @brief Vetor de chamadas de subida (Botões externos ^).
 * @note Definido em globals.c
 */
extern bool chamadas_subida[4];  

/**
 * @brief Vetor de chamadas de descida (Botões externos v).
 * @note Definido em globals.c
 */
extern bool chamadas_descida[4]; 

// VARIÁVEIS DE CONTROLE
extern uint16_t contador_telemetria;
extern uint16_t contador_espera;
extern char buffer_origem;
extern char buffer_destino;

#endif /* GLOBALS_H */