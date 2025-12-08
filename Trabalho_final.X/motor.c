/**
 * @file motor.c
 * @brief Driver do Motor (Versão Otimizada "Light" - Completa)
 * @details Implementa a lógica de movimentação, controle de velocidade, 
 * direção e proteções de hardware do elevador.
 */

#include "motor.h"
#include "globals.h"                
#include "mcc_generated_files/mcc.h" 
#include "mcc_generated_files/pwm3.h"


// =======================
// CONSTANTES E DEFINIÇÕES
// =======================

/** 
 * @brief Limite máximo de pulsos (segurança de software). 
 */
#define MAX_PULSOS_TOPO   220

/** 
 * @brief Posição física máxima em milímetros. 
 */
#define POSICAO_MAX_MM    180

/** 
 * @brief Fator de conversão: 0.837 mm/pulso * 1000 = 837. 
 */
#define MICRONS_POR_PULSO 837 

/** 
 * @brief Período de execução da tarefa de sensores (ms). 
 */
#define TEMPO_TMR4_MS     100  

// ===================
// VARIÁVEIS INTERNAS 
// ===================

/**
 * @brief Contador acumulativo de pulsos do encoder.
 * Utilizado para o cálculo de posição absoluta.
 */
static uint16_t total_pulsos = 0;            

/**
 * @brief Armazena o valor anterior do TMR0.
 * Usado para calcular o delta de pulsos entre chamadas.
 */
static uint8_t ultimo_valor_timer0 = 0;

// ====================
// CÁLCULO DOS SENSORES
// ====================

/**
 * @brief Realiza a telemetria do sistema (Velocidade, Posição e Temperatura).
 * @note Modifica as variáveis globais: #posicao_mm, #velocidade_atual e #temperatura_ponte.
 */
void SENSORES_CalcularVelocidade(void){
    
    // 1. LEITURA DO ENCODER
    // Lê o registrador TMR0 que conta os pulsos físicos do disco do motor
    uint8_t valor_atual = TMR0_ReadTimer();     

    // Calcula quantos pulsos aconteceram desde a última leitura
    uint8_t delta = valor_atual - ultimo_valor_timer0;
    
    // Salva o valor atual para a próxima conta
    ultimo_valor_timer0 = valor_atual;

    // 2. ATUALIZAÇÃO DA POSIÇÃO 
    if (estado_motor == MOTOR_SUBINDO) {
        total_pulsos += delta; // Se estiver subindo, soma-se os pulsos
        
        // Trava de segurança lógica
        if(total_pulsos > MAX_PULSOS_TOPO) total_pulsos = MAX_PULSOS_TOPO; 
    } 
    
    else if (estado_motor == MOTOR_DESCENDO) {
        if(delta > total_pulsos) total_pulsos = 0;  // Proteção para não ficar negativo
        else total_pulsos -= delta; // Se estiver descendo, subtraem-se os pulsos
    }
    
    // 3. CONVERSÃO MATEMÁTICA 
    // Conversão dos pulsos para milímetros 
    // Fórmula Otimizada: mm = (pulsos * 837) / 1000
    
    // É utilizada uma variável temporária de 32 bits para a multiplicação não estourar o limite de 16 bits
    uint32_t calculo_posicao = (uint32_t)total_pulsos * MICRONS_POR_PULSO;
    posicao_mm = (uint8_t)(calculo_posicao / 1000); // Guarda na variável global (0-180mm)

    // 4. CÁLCULO DA VELOCIDADE
    // Velocidade = Distância / Tempo
    // Fórmula utilizada: (delta * 837) / 100, velocidade em mm/s
    uint32_t calculo_velocidade = (uint32_t)delta * MICRONS_POR_PULSO;
    velocidade_atual = (uint8_t)(calculo_velocidade / 100);
    
    // 5. CÁLCULO DA TEMPERATURA
    uint16_t leitura_adc = 0;   
    
    // Média móvel simples para estabilizar a leitura do ADC
    for(int i = 0;  i < 10; i++){
        leitura_adc += ADC_GetConversion(channel_AN2);
        __delay_ms(100); 
    }
    
    // Média aritmética
    temperatura_ponte = (uint16_t)(leitura_adc / 10);
}

// ================================
// FUNÇÕES DE CONTROLE DE MOVIMENTO
// ================================

/**
 * @brief Envia comando para o motor subir.
 * @note Configura o PWM com duty cycle #MOTOR_ON e define #DIR como #DIRECAO_SUBIR.
 */
void Controle_Subir() {
    DIR = DIRECAO_SUBIR;          // Atualiza a variável DIR 
    PWM3_LoadDutyValue(MOTOR_ON); // Ativa o PWM
    estado_motor = MOTOR_SUBINDO; // Atualiza o estado lógico
}

/**
 * @brief Envia comando para o motor descer.
 * @note Configura o PWM com duty cycle #MOTOR_ON e define #DIR como #DIRECAO_DESCER.
 */
void Controle_Descer() {
    DIR = DIRECAO_DESCER;          // Atualiza a variável DIR
    PWM3_LoadDutyValue(MOTOR_ON);  // Ativa o PWM
    estado_motor = MOTOR_DESCENDO; // Atualiza o estado lógico
}

/**
 * @brief Para o motor imediatamente.
 * @note Zera o PWM (#MOTOR_OFF) e define estado como #MOTOR_PARADO.
 */
void Controle_Parar() {
    PWM3_LoadDutyValue(MOTOR_OFF); // Desativa o PWM
    estado_motor = MOTOR_PARADO;   // Atualiza o estado lógico
}

// ===============================
// LEITURA DE SENSORES E SEGURANÇA
// ===============================

/**
 * @brief Verifica os sensores de fim de curso e de andar.
 * @details Realiza a leitura dos sensores S1, S2, S3 e S4
 * para atualizar a variável global #andar_atual.
 * Também atua como segurança de hardware (Emergency Stop) caso o elevador
 * passe dos limites.
 */
void Verificar_Sensores() {
    
    // Atualiza andar atual
    // S1/S2: Digitais (Pull-up - Ativo em 0)
    if (SENSOR_S1 == 0) andar_atual = 0;
    if (SENSOR_S2 == 0) andar_atual = 1;
    // S3/S4: Analógicos (Comparador - Ativo em 1)
    if (SENSOR_S3 == 1) andar_atual = 2; 
    if (SENSOR_S4 == 1) andar_atual = 3; 

    // SEGURANÇA EXTREMA 
    // Se bater no chão descendo, motor para
    if (SENSOR_S1 == 0 && estado_motor == MOTOR_DESCENDO) {
        Controle_Parar();
        estado_atual = ESTADO_PARADO;
        posicao_mm = 0; // Recalibra a posição física para 0
    }
    // Se bater no teto subindo, motor para
    if (SENSOR_S4 == 1 && estado_motor == MOTOR_SUBINDO) {
        Controle_Parar();
        estado_atual = ESTADO_PARADO;
        posicao_mm = POSICAO_MAX_MM; // Recalibra a posição física para o máximo
    }
}

// =======================
// ALGORITMO DE OTIMIZAÇÃO
// =======================

/**
 * @brief Verifica se existem chamadas pendentes acima do andar de referência.
 * @param andar_ref O andar a partir do qual a busca será feita.
 * @return true Se houver qualquer chamada, subida ou descida, acima.
 * @return false Se não houver chamadas.
 */
bool Existe_Chamada_Acima(uint8_t andar_ref) {
    // Varre dos andares acima até o topo 
    for (int i = andar_ref + 1; i <= 3; i++) {
        // Verifica se há chamada de subida OU descida naquele andar
        if (chamadas_subida[i] || chamadas_descida[i]) return true;
    }
    return false;
}

/**
 * @brief Verifica se existem chamadas pendentes abaixo do andar de referência.
 * @param andar_ref O andar a partir do qual a busca será feita.
 * @return true Se houver qualquer chamada, subida ou descida, abaixo.
 * @return false Se não houver chamadas.
 */
bool Existe_Chamada_Abaixo(uint8_t andar_ref) {
    // Varre dos andares abaixo até o térreo
    for (int i = 0; i < andar_ref; i++) {
        // Verifica se há chamada de subida OU descida naquele andar
        if (chamadas_subida[i] || chamadas_descida[i]) return true;
    }
    return false;
}

/**
 * @brief Limpa a solicitação do andar atual após o atendimento.
 * @details Remove a pendência dos vetores globais (#chamadas_subida ou #chamadas_descida)
 * baseando-se na direção atual do elevador e nas regras de fim de curso.
 */
void Limpar_Chamada_Atual() {
    
    // Se estava subindo ou parado, marca como atendida a solicitação de subida
    if (estado_motor == MOTOR_SUBINDO || estado_motor == MOTOR_PARADO) {
        chamadas_subida[andar_atual] = false;
    }
    
    // Se estava descendo ou parado, marca como atendida a solicitação de descida
    if (estado_motor == MOTOR_DESCENDO || estado_motor == MOTOR_PARADO) {
        chamadas_descida[andar_atual] = false;
    }
    
    // Tratamento de Extremos
    // No último andar, limpa forçado
    if (andar_atual == 3) chamadas_subida[3] = false; 
    
    // No térreo, limpa forçado
    if (andar_atual == 0) chamadas_descida[0] = false;
}

