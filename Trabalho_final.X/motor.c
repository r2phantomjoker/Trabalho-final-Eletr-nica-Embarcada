/**
 @file motor.c
 Autor: Arthur Marinho
 @brief Driver do Motor (Versão Otimizada "Light" - Completa)
 Implementa a lógica de movimentação, controle de velocidade (PWM),direção e proteções de hardware (reversão brusca e fim de curso).
*/

#include "motor.h"
#include "globals.h"               
#include "mcc_generated_files/mcc.h" 
#include "mcc_generated_files/pwm3.h"

// VARIÁVEIS INTERNAS (OTIMIZADAS PARA INTEIROS)
// Usamos 'static' para que elas só existam dentro deste arquivo.

// Antes era float. Mudamos para uint16 para economizar memória RAM e FLASH.
static uint16_t total_pulsos = 0;            

// Guarda a leitura anterior do Timer para saber o quanto andou (Delta).
static uint8_t ultimo_valor_timer0 = 0;

// Constante: 0.837 mm/pulso * 1000 = 837.
// Usamos isso para fazer conta com números inteiros e fugir das bibliotecas pesadas de 'float'.
#define MICRONS_POR_PULSO 837 

#define TEMPO_TMR4_MS    100  // Timer 4 roda a cada 100ms

// ÁREA DESATIVADA (CÓDIGO MORTO)
// Mantido aqui para referência, mas comentado para o compilador ignorar.

/* INÍCIO DO BLOCO COMENTADO

static uint8_t ultima_direcao_aplicada = DIRECAO_SUBIR;
static float posicao_mm_fina = 0.0;            
static uint16_t pulsos_para_velocidade = 0;

// Esta função foi substituída por Controle_Parar() no main.c
void MOTOR_parar(void){
    estado_motor = MOTOR_PARADO;
}

// Esta função foi substituída pela lógica de "Homing" no main.c
void MOTOR_reset(void){
    
    // 1. DETECÇÃO INICIAL DE POSIÇÃO
    // Verifica os sensores físicos para estimar onde o elevador ligou.
    // S1/S2 são Active Low (0). S3/S4 são Active High (1) via Comparador.
    
    if(SENSOR_S1 == 0){
        andar_atual = 0;
    }
    else if (SENSOR_S2 == 0){
        andar_atual = 1;
    }
    else if(SENSOR_S3 == 1){
        andar_atual = 2;
    }
    else if(SENSOR_S4 == 1){ 
        andar_atual = 3;
    }
    else {
        // Caso nenhum sensor esteja ativado (Elevador entre andares)
        andar_atual = 255; 
    }
   
    // 2. MOVIMENTAÇÃO PARA O TÉRREO
    if(andar_atual != 0){
        andar_destino = 0;
        
        // Se estiver "perdido" (255), simulamos que estamos no topo (4) para garantir que a lógica decida DESCER. uint8_t origem_simulada = (andar_atual == 255) ? 4 : andar_atual;
        
        MOTOR_mover(andar_destino, origem_simulada);
    }
    
    // 3. RESET DE VARIÁVEIS
    MOTOR_parar();
    andar_atual = 0;
    posicao_mm = 0;
    andar_destino = 0;
    estado_motor = MOTOR_PARADO;
    andar_destino = 0;
    
    // Zera o hardware do Timer 0 também para começar limpo
    TMR0_WriteTimer(0);
    ultimo_valor_timer0 = 0;
    
    // Limpa a fila de chamadas
    for(uint8_t i=0; i<4; i++) {
        solicitacoes[i] = false;
    }
}

// Esta função causava travamento no envio do Bluetooth (loop while).
// A lógica dela foi quebrada e levada para a Máquina de Estados no main.c.
void MOTOR_mover (uint8_t destino, uint8_t atual)
{
    // 1. VALIDAÇÕES DE SEGURANÇA (BOUNDS CHECK)
    if (atual > 3 && atual != 255) { // Proteção contra valores corrompidos
        MOTOR_parar(); return;
    }
    if (destino == atual){ // O elevador está no andar do destino
        MOTOR_parar(); return;
    }
   
    // 2. DECISÃO DE DIREÇÃO
    // Apenas decide a intenção, NÃO aplica no pino ainda.
    uint8_t nova_direcao;

    if(destino > atual){
        nova_direcao = DIRECAO_SUBIR;
    }
    else {
        nova_direcao = DIRECAO_DESCER;
    }
   
    // 3. PROTEÇÃO DE HARDWARE (PONTE H)
    // Regra do Roteiro: Esperar 500ms se inverter o sentido em movimento.
    if ((estado_motor != MOTOR_PARADO) && (nova_direcao != ultima_direcao_aplicada)) {
        MOTOR_parar();      
        __delay_ms(500); // Delay comum é seguro agora (TMR0 conta no fundo)
    }
   
    // 4. APLICAÇÃO NO HARDWARE
    // Agora é seguro mudar o pino DIR.
    if(nova_direcao == DIRECAO_SUBIR){
        DIR = DIRECAO_SUBIR;
        estado_motor = MOTOR_SUBINDO;
    }
    else {
        DIR = DIRECAO_DESCER;
        estado_motor = MOTOR_DESCENDO;
    }
   
    ultima_direcao_aplicada = nova_direcao;
   
    // 5. LOOP DE MOVIMENTO (BLOQUEANTE)
    // Mantém o motor ligado até chegar no destino final.
    while(atual != destino){
       
        PWM3_LoadDutyValue(MOTOR_ON); // Liga o motor (~60%)
       
        // Loop de Espera: Aguarda até ALGUM sensor ser acionado
        // S1/S2 = 1 (Desativado), S3/S4 = 0 (Desativado)
        while( (SENSOR_S1 == 1) && (SENSOR_S2 == 1) && 
               (SENSOR_S3 == 0) && (SENSOR_S4 == 0) ) 
        {
           
            // Proteção de Fim de Curso:
            // Se estiver descendo e bater no S1, sai do loop imediatamente.
            if (DIR == DIRECAO_DESCER && SENSOR_S1 == 0) break;
            
            // Se estiver subindo e bater no S4, sai do loop imediatamente.
            if (DIR == DIRECAO_SUBIR && SENSOR_S4 == 1) break;
        }
       
        // Sensor detectado! Para o motor para atualizar posição.
        MOTOR_parar();
        
        // Atualiza a variável 'atual' matematicamente para o próximo passo
        if(DIR == DIRECAO_DESCER) atual--;
        else if (DIR == DIRECAO_SUBIR) atual++;
       
        __delay_ms(100);
    }
   
    // Chegou no destino final
    MOTOR_parar();
    andar_atual = atual; // Sincroniza a variável global
}

FIM DO BLOCO COMENTADO 
*/

// CÁLCULO DE FÍSICA (VELOCIDADE E POSIÇÃO) - CÓDIGO ATIVO E OTIMIZADO

/**
 * @brief Calcula Velocidade e Posição lendo o Hardware (TMR0).
 * @note Deve ser chamada periodicamente (ex: Timer 4 a cada 100ms).
 */
void SENSORES_CalcularVelocidade(void){
    
    // 1. LEITURA DO ENCODER (HARDWARE)
    // Lê o registrador TMR0 que conta os pulsos físicos do disco do motor.
    uint8_t valor_atual = TMR0_ReadTimer();
    
    // Calcula quantos pulsos aconteceram nesses 100ms (Atual - Anterior).
    // O tipo uint8_t lida automaticamente com o estouro (ex: se foi de 250 para 5, o resultado é 11).
    uint8_t delta = valor_atual - ultimo_valor_timer0;
    
    // Salva o valor atual para a próxima conta.
    ultimo_valor_timer0 = valor_atual;

    // 2. ATUALIZAÇÃO DA POSIÇÃO (CONTAGEM DE PULSOS)
    // Se o motor estiver subindo, somamos os pulsos.
    if (estado_motor == MOTOR_SUBINDO) {
        total_pulsos += delta;
        // Trava de segurança lógica: 220 pulsos é o topo (~180mm).
        // Impede que o número cresça infinitamente se o sensor falhar.
        if(total_pulsos > 220) total_pulsos = 220; 
    } 
    // Se estiver descendo, subtraímos.
    else if (estado_motor == MOTOR_DESCENDO) {
        // Proteção para não ficar negativo (o que seria um erro em 'unsigned').
        if(delta > total_pulsos) total_pulsos = 0; 
        else total_pulsos -= delta;
    }
    
    // 3. CONVERSÃO MATEMÁTICA (INTEIROS)
    // Aqui transformamos "pulsos" em "milímetros" para o celular mostrar.
    // Fórmula Otimizada: mm = (pulsos * 837) / 1000
    
    // Usamos uma variável temporária de 32 bits (uint32) para a multiplicação não estourar o limite de 16 bits.
    uint32_t calculo_posicao = (uint32_t)total_pulsos * MICRONS_POR_PULSO;
    posicao_mm = (uint8_t)(calculo_posicao / 1000); // Guarda na variável global (0-180mm)

    // 4. CÁLCULO DA VELOCIDADE
    // Velocidade = Distância / Tempo. Como o tempo é fixo (0.1s), a conta simplifica.
    // Truque matemático: (delta * 837) / 100 dá a velocidade já na escala mm/s.
    uint32_t calculo_velocidade = (uint32_t)delta * MICRONS_POR_PULSO;
    velocidade_atual = (uint8_t)(calculo_velocidade / 100);
}