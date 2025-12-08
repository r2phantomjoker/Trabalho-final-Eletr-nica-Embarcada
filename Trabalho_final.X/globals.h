/**
 * @file globals.h
 * @brief Definições globais, constantes de hardware e variáveis de estado do Elevador.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <xc.h>         
#include <stdint.h>     
#include <stdbool.h>    


// ======================
// MAPEAMENTO DE HARDWARE 
// ======================

/**
 * @brief Definição dos estados físicos do motor.
 */
#define MOTOR_PARADO    0   // Motor desligado 
#define MOTOR_SUBINDO   1   // Movimento ascendente 
#define MOTOR_DESCENDO  2   // Movimento descendente 


/**
 * @brief Controle do Chip Select do Driver MAX7219.
 */
#define CS_SetHigh()    LATBbits.LATB1 = 1 // Desabilita comunicação (Latch)
#define CS_SetLow()     LATBbits.LATB1 = 0 // Habilita comunicação (Start)

/**
 * @brief Sensores Digitais.
 * Tipo: Sensor de Efeito Hall - Coletor Aberto.
 * @note Lógica - Active low:
 * - 1 : Sem ímã - Elevador longe.
 * - 0 : Com ímã - Elevador no andar.
 */
#define SENSOR_S1       PORTBbits.RB0   // Sensor do Térreo 
#define SENSOR_S2       PORTBbits.RB3   // Sensor do 1º Andar

/**
 * @brief Sensores Analógicos.
 * Lidos através dos Comparadores Analógicos.
 * @note Lógica - Active HIGH:
 * - 0 : Sem ímã (Elevador longe).
 * - 1 : Com ímã (Elevador detectado).
 */
#define SENSOR_S3       CM1CON0bits.C1OUT  // Sensor do 2º Andar 
#define SENSOR_S4       CM2CON0bits.C2OUT  // Sensor do 3º Andar 


/**
 * @brief Controle do Pino de Direção - RA7.
 */
#define DIR             LATAbits.LATA7 
#define DIRECAO_SUBIR   1   // Nível Alto 
#define DIRECAO_DESCER  0   // Nível Baixo

/**
 * @brief Valores de PWM .
 * @note 614 = 60% de Duty Cycle.
 */
#define MOTOR_OFF       0    // Motor desligado
#define MOTOR_ON        614  // Motor ligado (~60%)

// =================
// VARIÁVEIS GLOBAIS 
// =================

/**
 * @brief Andar atual onde o elevador se encontra.
 * @note Faixa: 0 a 3.
 */
extern volatile uint8_t andar_atual;

/**
 * @brief Andar de destino da solicitação atual.
 * @note Faixa: 0 a 3.
 */
extern volatile uint8_t andar_destino;

/**
 * @brief Estado físico do motor.
 * @note Valores: 0 (Parado), 1 (Subindo), 2 (Descendo).
 */
extern volatile uint8_t estado_motor;

/**
 * @brief Posição estimada em milímetros.
 * @note Faixa: 0 a 180 mm.
 */
extern volatile uint8_t posicao_mm;

/**
 * @brief Velocidade instantânea.
 * @note Unidade: mm/s.
 */
extern volatile uint8_t velocidade_atual;

/**
 * @brief Temperatura monitorada na Ponte H.
 * @note Unidade:°C.
 */
extern volatile uint16_t temperatura_ponte;


/**
 * @brief Vetor unificado de solicitações para o Display.
 * @note Índice [0]=Térreo, [1]=1º Andar, [2]=2º Andar e [3]=3º Andar.
 * true = LED deve acender, solicitação ativa.
 */
extern volatile bool solicitacoes[4];


/**
 * @brief Estados possíveis da Máquina de Estados.
 */
typedef enum {
    ESTADO_PARADO,
    ESTADO_SUBINDO,
    ESTADO_DESCENDO,
    ESTADO_ESPERA_PORTA,
    ESTADO_REVERSAO
} EstadoElevador;

/**
 * @brief Estado atual do elevador.
 */
extern volatile EstadoElevador estado_atual;

/**
 * @brief Vetor de chamadas de subida.
 */
extern bool chamadas_subida[4];  

/**
 * @brief Vetor de chamadas de descida.
 */
extern bool chamadas_descida[4]; 

/**
 * @brief Contador para divisão de frequência da Telemetria.
 */
extern uint16_t contador_telemetria;

/**
 * @brief Contador para temporizações da Máquina de Estados.
 */
extern uint16_t contador_espera;

/**
 * @brief Buffer temporário para o andar de origem.
 */
extern char buffer_origem;   

/**
 * @brief Buffer temporário para o andar de destino.
 */
extern char buffer_destino;  

#endif