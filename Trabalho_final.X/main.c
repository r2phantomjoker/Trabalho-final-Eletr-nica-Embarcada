/**
 * @file main.c
 * @brief Lógica Central (Cérebro): Máquina de Estados e Otimização
 * Integração Final - Versão Pronta para Hardware
 */

#include "mcc_generated_files/mcc.h"
#include "globals.h"
#include "comm.h"
#include "motor.h"

// Esperando o Gabriel fazer a matriz de LED:
// #include "max7219.h"

// ESTADOS DA MÁQUINA DE ESTADOS FINITOS (FSM)
typedef enum {
    ESTADO_PARADO,
    ESTADO_SUBINDO,
    ESTADO_DESCENDO,
    ESTADO_ESPERA_PORTA,
    ESTADO_REVERSAO
} EstadoElevador;

EstadoElevador estado_atual = ESTADO_PARADO;

// Variáveis de controle de tempo e buffer
uint16_t contador_telemetria = 0;
uint16_t contador_espera = 0;
char buffer_origem, buffer_destino;

// 1. FUNÇÕES DE CONTROLE DE MOVIMENTO
void Controle_Subir() {
    DIR = DIRECAO_SUBIR;           
    PWM3_LoadDutyValue(MOTOR_ON);
    estado_motor = MOTOR_SUBINDO;
}

void Controle_Descer() {
    DIR = DIRECAO_DESCER;          
    PWM3_LoadDutyValue(MOTOR_ON);  
    estado_motor = MOTOR_DESCENDO;
}

void Controle_Parar() {
    PWM3_LoadDutyValue(MOTOR_OFF); 
    estado_motor = MOTOR_PARADO;
}

// 2. LEITURA DE SENSORES E SEGURANÇA
void Verificar_Sensores() {
    // Atualiza andar atual
    // S1 e S2: Sensores Digitais (Pull-up -> Ativo em 0)
    if (SENSOR_S1 == 0) andar_atual = 0;
    if (SENSOR_S2 == 0) andar_atual = 1;
    
    // S3 e S4: Sensores Analógicos via Comparador (Ativo em 1)
    if (SENSOR_S3 == 1) andar_atual = 2; 
    if (SENSOR_S4 == 1) andar_atual = 3; 

    // SEGURANÇA EXTREMA (Fim de Curso)
    // Se bater no chão descendo -> PARA TUDO
    if (SENSOR_S1 == 0 && estado_motor == MOTOR_DESCENDO) {
        Controle_Parar();
        estado_atual = ESTADO_PARADO;
        posicao_mm = 0; // Recalibra posição zero
    }
    // Se bater no teto subindo -> PARA TUDO
    if (SENSOR_S4 == 1 && estado_motor == MOTOR_SUBINDO) {
        Controle_Parar();
        estado_atual = ESTADO_PARADO;
        posicao_mm = 180; // Recalibra topo
    }
}

// 3. ALGORITMO DE OTIMIZAÇÃO (O Cérebro)
int Buscar_Proxima_Parada() {
    // 1. Se parado, atende qualquer solicitação
    if (estado_atual == ESTADO_PARADO) {
        for (int i = 0; i < 4; i++) if (solicitacoes[i]) return i;
        return -1;
    }
    // 2. Se subindo, prioriza quem está ACIMA do andar atual
    if (estado_atual == ESTADO_SUBINDO) {
        for (int i = andar_atual + 1; i <= 3; i++) {
            if (solicitacoes[i]) return i;
        }
    }
    // 3. Se descendo, prioriza quem está ABAIXO do andar atual
    if (estado_atual == ESTADO_DESCENDO) {
        for (int i = andar_atual - 1; i >= 0; i--) {
            if (solicitacoes[i]) return i;
        }
    }
    // 4. Se não achou no sentido, varre tudo (permitirá inversão depois)
    for (int i = 0; i < 4; i++) if (solicitacoes[i]) return i;
    
    return -1;
}

// PROGRAMA PRINCIPAL

void main(void) {
    // Inicializa Hardware (MCC)
    SYSTEM_Initialize(); 

    // CORREÇÃO DE CONFLITO DE HARDWARE
    // Desabilita interrupção por mudança de estado (IOC) nos sensores S1/S2.
    // Isso evita que o processador trave tentando atender interrupções vazias enquanto nós lemos os sensores manualmente por polling.
    INTCONbits.IOCIE = 0; 

    // Inicializa Matriz de LEDs (Futuro)
    // MAX7219_Init();

    // Liga interrupção do Timer 4 para cálculo de velocidade (Encoder)
    TMR4_SetInterruptHandler(SENSORES_CalcularVelocidade);

    // Habilita Interrupções Globais
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    // Estado inicial seguro
    Controle_Parar(); 

    while (1) {
        // A. COMUNICAÇÃO (BLUETOOTH)
        if(EUSART_is_rx_ready()) {
            // Tenta ler o pacote $OD<cr>
            if (UART_RecebePedido(&buffer_origem, &buffer_destino) == 0) { 
                int o = buffer_origem - '0';
                int d = buffer_destino - '0';
                
                // Validação de segurança dos andares (0 a 3)
                if (o >= 0 && o <= 3) solicitacoes[o] = true;
                if (d >= 0 && d <= 3) solicitacoes[d] = true;
            }
        }

        // B. SENSORES (OLHOS)
        Verificar_Sensores();

        // C. MÁQUINA DE ESTADOS (LÓGICA)
        switch (estado_atual) {
            case ESTADO_PARADO: {
                int alvo = Buscar_Proxima_Parada();
                
                if (alvo != -1) {
                    andar_destino = alvo; // Define para onde ir
                    
                    if (andar_destino > andar_atual) {
                        Controle_Subir();
                        estado_atual = ESTADO_SUBINDO;
                    } else if (andar_destino < andar_atual) {
                        Controle_Descer();
                        estado_atual = ESTADO_DESCENDO;
                    } else {
                        // Já está no andar solicitado (abre a porta)
                        solicitacoes[alvo] = false; 
                        estado_atual = ESTADO_ESPERA_PORTA;
                        contador_espera = 0;
                    }
                } else {
                    // Homing: Se ocioso e fora do térreo, volta pro 0 (Repouso)
                    if (andar_atual != 0) solicitacoes[0] = true;
                }
                break;
            }
            
            case ESTADO_SUBINDO:
            case ESTADO_DESCENDO:
                // OTIMIZAÇÃO EM TEMPO REAL
                // Verifica a cada ciclo se apareceu uma chamada mais próxima no caminho.
                // Isso permite a "Carona" (ex: parar no 2 indo pro 3).
                int novo_alvo = Buscar_Proxima_Parada();
                if (novo_alvo != -1) {
                    andar_destino = novo_alvo;
                }

                // Verifica se chegou no destino
                if (andar_atual == andar_destino) {
                    Controle_Parar();
                    solicitacoes[andar_atual] = false; // Atende o pedido
                    estado_atual = ESTADO_ESPERA_PORTA;
                    contador_espera = 0;
                }
                break;

            case ESTADO_ESPERA_PORTA:
                contador_espera++;
                // Espera ~2 segundos (200 * 10ms)
                if (contador_espera >= 200) { 
                    int proximo = Buscar_Proxima_Parada();
                    
                    if (proximo != -1) {
                        // Lógica de Proteção de Reversão:
                        // Se for necessário inverter o sentido do motor imediatamente, passamos pelo estado de REVERSAO para dar um tempo de descanso.
                        bool inverte = false;
                        
                        // Nota: estado_motor aqui é PARADO (0). A lógica simplificada assume que se vamos mudar de sentido, é bom esperar.
                        // Aqui, forçamos um check rápido:
                        estado_atual = ESTADO_REVERSAO;
                        contador_espera = 0;
                    } else {
                        estado_atual = ESTADO_PARADO;
                    }
                }
                break;

            case ESTADO_REVERSAO:
                contador_espera++;
                // Deadtime de 500ms para proteger a Ponte H
                if (contador_espera >= 50) { 
                    estado_atual = ESTADO_PARADO;
                }
                break;
        }

        // D. TELEMETRIA E DISPLAY
        contador_telemetria++;
        if (contador_telemetria >= 30) { // A cada ~300ms
            UART_EnviaDados(); // Envia para o celular
            
            // Futuro: Atualizar Matriz de LEDs
            // MAX7219_AtualizarDisplay(andar_atual, estado_motor);
            
            contador_telemetria = 0; 
        }

        // Loop de aproximadamente 10ms
        __delay_ms(10);
    }
}