/*
 * Arquivo: globals.c
 * Descrição: Alocação real de memória das variáveis globais
 * Autor: Arthur Marinho
 */

#include "globals.h"


// Variáveis de Telemetria 
volatile uint8_t andar_atual = 0;       // Inicia no Térreo (0)
volatile uint8_t andar_destino = 0;     // Sem destino inicial
volatile uint8_t estado_motor = MOTOR_PARADO; 
volatile uint8_t posicao_mm = 0;        // 0 mm (Térreo)
volatile uint8_t velocidade_atual = 0;  // 0.0*10 mm/s
volatile uint16_t temperatura_ponte = 0; // 0.0*10 °C

// Vetor de Solicitações (Botões apertados)
// [0]=Térreo, [1]=1ºAndar, [2]=2ºAndar, [3]=3ºAndar
volatile bool solicitacoes[4] = {false, false, false, false};

// Estado da Máquina de Estados
volatile EstadoElevador estado_atual = ESTADO_PARADO;



// Aqui as variáveis são realmente criadas na RAM.
// Note: Não use 'extern' aqui.

bool chamadas_subida[4]  = {false, false, false, false};
bool chamadas_descida[4] = {false, false, false, false};

uint16_t contador_telemetria = 0;
uint16_t contador_espera = 0;

// Variáveis não inicializadas explicitamente são 0 por padrão, 
// mas é boa prática inicializar.
char buffer_origem = 0;
char buffer_destino = 0;
