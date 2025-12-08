/**
 * @file globals.c
 * @brief Alocação real de memória das variáveis globais do sistema.
 * @details Este arquivo aloca e define os valores iniciais das variáveis.
 */

#include "globals.h"


/** 
 * @brief Inicia o sistema assumindo que o elevador está no Térreo. 
 */
volatile uint8_t andar_atual = 0;       

/** 
 * @brief Destino inicial nulo até receber comando. 
 */
volatile uint8_t andar_destino = 0;     

/** 
 * @brief Estado físico inicial do motor como parado.
 */
volatile uint8_t estado_motor = MOTOR_PARADO; 

/** 
 * @brief Posição absoluta inicial como 0 mm. 
 */
volatile uint8_t posicao_mm = 0;        

/** 
 * @brief Velocidade inicial 0 mm/s. 
 */
volatile uint8_t velocidade_atual = 0;  

/** 
 * @brief Temperatura inicial zerada. 
 */
volatile uint16_t temperatura_ponte = 0; 

/**
 * @brief Vetor de visualização de chamadas.
 * Inicializado com todos os andares 'false', sem chamadas.
 */
volatile bool solicitacoes[4] = {false, false, false, false};

/** 
 * @brief Estado inicial da Máquina de Estados Lógica. 
 */
volatile EstadoElevador estado_atual = ESTADO_PARADO;


/**
 * @brief Filas de processamento do algoritmo SCAN.
 * Armazenam as requisições pendentes de subida e descida separadamente.
 */
bool chamadas_subida[4]  = {false, false, false, false};
bool chamadas_descida[4] = {false, false, false, false};


/** 
 * @brief Inicializa o contador de tempo de telemetria zerado. 
 */
uint16_t contador_telemetria = 0;

/** 
 * @brief Inicializa o contador de temporização de estados zerado. 
 */
uint16_t contador_espera = 0;

/**
 * @brief Buffers de recepção da UART.
 * Inicializados com 0 por segurança.
 */
char buffer_origem = 0;
char buffer_destino = 0;