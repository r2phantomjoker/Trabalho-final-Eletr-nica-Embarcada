/**
 * @file motor.c
 * @author Arthur Marinho
 * @brief Driver de controle de baixo nível para a Ponte H TB6612FNG.
 *
 * Implementa a lógica de movimentação, controle de velocidade (PWM),
 * direção e proteções de hardware (reversão brusca e fim de curso).
 */

#include "motor.h"
#include "globals.h"               // Para atualizar estado_motor 
#include "mcc_generated_files/mcc.h" // Para usar PWM e Delay

/** * @brief Armazena a última direção aplicada para evitar reversão brusca.
 * Inicializa como SUBINDO por segurança.
 */
static uint8_t ultima_direcao_aplicada = DIRECAO_SUBIR;

/** * @brief Acumulador de posição com precisão decimal (float).
 * Usado internamente para não perder os 0.83mm a cada pulso.
 */
static float posicao_mm_fina = 0.0;            

/** * @brief Contador de pulsos para o cálculo de velocidade.
 * Reiniciado a cada ciclo do Timer 4 (100ms).
 */
static uint16_t pulsos_para_velocidade = 0;

/**
 * @brief Armazena a leitura anterior do registrador TMR0.
 * @note Usada para calcular o diferencial (Delta) de pulsos entre
 * duas chamadas do Timer de velocidade (Pulsos Atuais - Pulsos Anteriores).
 */
static uint8_t ultimo_valor_timer0 = 0;

/**
 * @brief Fator de conversão de Pulsos para Milímetros.
 * Cálculo: 180mm  / 215 pulsos  = 0.8372 mm/pulso.
 */
#define MM_POR_PULSO  0.8372f 

/**
 * @brief Janela de tempo para cálculo de velocidade.
 * Definido pela configuração do Timer 4 no MCC (100ms = 0.1s).
 */
#define TEMPO_TMR4    0.1f


/**
 * @brief Para o motor imediatamente.
 * Define o Duty Cycle do PWM para 0 e atualiza o estado global.
 */
void MOTOR_parar(void){
    PWM3_LoadDutyValue(MOTOR_OFF); 
    estado_motor = MOTOR_PARADO;
}

/**
 * @brief Realiza o procedimento de Calibração Inicial.
 * 1. Detecta o andar atual lendo os sensores diretamente.
 * 2. Se não estiver no térreo, força a descida até encontrar o sensor S1.
 * 3. Zera todas as variáveis de posição e solicitações.
 * @note Esta função é bloqueante (trava o sistema até calibrar).
 */
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
        
        // Se estiver perdido, simulamos que estamos no topo (4)
        // para garantir que a lógica decida DESCER.
        uint8_t origem_simulada = (andar_atual == 255) ? 4 : andar_atual;
        
        MOTOR_mover(andar_destino, origem_simulada);
    }
    
    // 3. RESET DE VARIÁVEIS
    MOTOR_parar();
    andar_atual = 0;
    posicao_mm = 0;
    andar_destino = 0;
    estado_motor = MOTOR_PARADO;
    andar_destino = 0;
    
    // Zera o hardware do Timer 0 
    TMR0_WriteTimer(0);
    ultimo_valor_timer0 = 0;
    
    
    
    // Limpa a fila de chamadas
    for(uint8_t i=0; i<4; i++) {
        solicitacoes[i] = false;
    }
}

/**
 * @brief Move o elevador de um andar de origem até o destino.
 * * Implementa lógica "Passo a Passo": o elevador viaja andar por andar,
 * parando brevemente em cada sensor até atingir o destino.
 * Inclui proteção de 500ms contra reversão de sentido.
 * * @param destino Andar alvo (0 a 3).
 * @param atual Andar atual (0 a 3, ou 255 se desconhecido).
 */
void MOTOR_mover (uint8_t destino, uint8_t atual)
{
    //1. VALIDAÇÕES DE SEGURANÇA 
    if (atual > 3 && atual != 255) { // Proteção contra valores desconhecidos
        MOTOR_parar(); return;
    }
    if (destino == atual){ // O elevador está no andar do destino
        MOTOR_parar(); return;
    }
   
    // 2. DECISÃO DE DIREÇÃO ---
    // Apenas decide a intenção, NÃO aplica no pino ainda.
    uint8_t nova_direcao;

    if(destino > atual){
        nova_direcao = DIRECAO_SUBIR;
    }
    else {
        nova_direcao = DIRECAO_DESCER;
    }
   
    //3. PROTEÇÃO DE HARDWARE
    // Esperar 500ms se inverter o sentido em movimento
    if ((estado_motor != MOTOR_PARADO) && (nova_direcao != ultima_direcao_aplicada)) {
        MOTOR_parar();      
        __delay_ms(500); 
    }
   
    // 4. APLICAÇÃO NO HARDWARE 
    if(nova_direcao == DIRECAO_SUBIR){
        DIR = DIRECAO_SUBIR;
        estado_motor = MOTOR_SUBINDO;
    }
    else {
        DIR = DIRECAO_DESCER;
        estado_motor = MOTOR_DESCENDO;
    }
   
    ultima_direcao_aplicada = nova_direcao;
   
    // 5. LOOP DE MOVIMENTO 
    while(atual != destino){
       
        PWM3_LoadDutyValue(MOTOR_ON); // Liga o motor (40%)
       
        // Aguarda até ALGUM sensor ser acionado
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
        
        // Atualiza a variável 'atual' para o próximo passo
        if(DIR == DIRECAO_DESCER) atual--;
        else if (DIR == DIRECAO_SUBIR) atual++;
       
        __delay_ms(100);
    }
   
    // Chegou no destino final
    MOTOR_parar();
    andar_atual = atual; // Sincroniza a variável global
}

/**
 * @brief Calcula Velocidade e Posição lendo o Hardware (TMR0).
 * * Funcionamento:
 * 1. Lê o registrador TMR0 (que conta pulsos do pino RA4).
 * 2. Calcula a diferença (Delta) desde a última leitura.
 * 3. Atualiza a posição (mm) e velocidade (mm/s).
 */
void SENSORES_CalcularVelocidade(void){
    
    // 1. Lê o registrador de hardware do Timer 0 
    uint8_t valor_atual_timer0 = TMR0_ReadTimer();
    
    // 2. Calcula quantos pulsos ocorreram desde a última vez 
    uint8_t delta_pulsos = valor_atual_timer0 - ultimo_valor_timer0;
    
    // Atualiza a memória para o próximo ciclo
    ultimo_valor_timer0 = valor_atual_timer0;

    // 3. Calcula Distância percorrida na janela (mm)
    float distancia_janela = (float)delta_pulsos * MM_POR_PULSO;
    
    // 4. Atualização da Posição Global 
    if (estado_motor == MOTOR_SUBINDO) {
        posicao_mm_fina += distancia_janela;
        
        // Trava lógica se chegar no teto 
        if (posicao_mm_fina > 180.0f) posicao_mm_fina = 180.0f;
    } 
    else if (estado_motor == MOTOR_DESCENDO) {
        posicao_mm_fina -= distancia_janela;
        
        // Trava lógica se chegar no chão 
        if (posicao_mm_fina < 0.0f) posicao_mm_fina = 0.0f;
    }
    
    // Atualiza variável global inteira 
    posicao_mm = (uint8_t)posicao_mm_fina;

    // 5. Cálculo da Velocidade Instantânea 
    velocidade_atual = (uint8_t)((distancia_janela / TEMPO_TMR4)*100);
}
