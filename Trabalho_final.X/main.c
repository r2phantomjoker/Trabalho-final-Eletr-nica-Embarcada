/**
 * @file main.c
 * @brief Lógica Central (Cérebro): Máquina de Estados e Otimização
 * @version Correção de Prioridade na Parada - falta matriz de LEDs
 */

#include "mcc_generated_files/mcc.h"
#include "globals.h"
#include "comm.h"
#include "motor.h"

// Quando tiver a matriz, lembrar de descomentar:
// #include "max7219.h"

// ESTADOS DO SISTEMA
typedef enum {
    ESTADO_PARADO,
    ESTADO_SUBINDO,
    ESTADO_DESCENDO,
    ESTADO_ESPERA_PORTA,
    ESTADO_REVERSAO
} EstadoElevador;

EstadoElevador estado_atual = ESTADO_PARADO;

// VARIÁVEIS DE CONTROLE
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
    // S1/S2: Digitais (Pull-up - Ativo em 0)
    if (SENSOR_S1 == 0) andar_atual = 0;
    if (SENSOR_S2 == 0) andar_atual = 1;
    // S3/S4: Analógicos (Comparador - Ativo em 1)
    if (SENSOR_S3 == 1) andar_atual = 2; 
    if (SENSOR_S4 == 1) andar_atual = 3; 

    // SEGURANÇA EXTREMA (Fim de Curso)
    // Se bater no chão descendo - PARA
    if (SENSOR_S1 == 0 && estado_motor == MOTOR_DESCENDO) {
        Controle_Parar();
        estado_atual = ESTADO_PARADO;
        posicao_mm = 0; // Recalibra
    }
    // Se bater no teto subindo - PARA
    if (SENSOR_S4 == 1 && estado_motor == MOTOR_SUBINDO) {
        Controle_Parar();
        estado_atual = ESTADO_PARADO;
        posicao_mm = 180; // Recalibra
    }
}

// 3. ALGORITMO DE OTIMIZAÇÃO
int Buscar_Proxima_Parada() {
    // 1. Se parado, atende qualquer um
    if (estado_atual == ESTADO_PARADO) {
        for (int i = 0; i < 4; i++) if (solicitacoes[i]) return i;
        return -1;
    }
    // 2. Se subindo, prioriza quem está ACIMA
    if (estado_atual == ESTADO_SUBINDO) {
        for (int i = andar_atual + 1; i <= 3; i++) {
            if (solicitacoes[i]) return i;
        }
    }
    // 3. Se descendo, prioriza quem está ABAIXO
    if (estado_atual == ESTADO_DESCENDO) {
        for (int i = andar_atual - 1; i >= 0; i--) {
            if (solicitacoes[i]) return i;
        }
    }
    // 4. Se não achou no sentido, varre tudo
    for (int i = 0; i < 4; i++) if (solicitacoes[i]) return i;
    
    return -1;
}

// LOOP PRINCIPAL
void main(void) {
    SYSTEM_Initialize(); // Inicializa drivers do MCC

    // PROTEÇÃO DE HARDWARE
    // Desliga interrupção IOC dos sensores para evitar conflito com Polling
    INTCONbits.IOCIE = 0; 

    // Liga timer do velocímetro
    TMR4_SetInterruptHandler(SENSORES_CalcularVelocidade);

    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    Controle_Parar(); 

    while (1) {
        // A. COMUNICAÇÃO BLUETOOTH
        if(EUSART_is_rx_ready()) {
            if (UART_RecebePedido(&buffer_origem, &buffer_destino) == 0) { 
                int o = buffer_origem - '0';
                int d = buffer_destino - '0';
                if (o >= 0 && o <= 3) solicitacoes[o] = true;
                if (d >= 0 && d <= 3) solicitacoes[d] = true;
            }
        }

        // B. SENSORES
        Verificar_Sensores();

        // C. MÁQUINA DE ESTADOS
        switch (estado_atual) {
            case ESTADO_PARADO: {
                int alvo = Buscar_Proxima_Parada();
                if (alvo != -1) {
                    andar_destino = alvo;
                    if (andar_destino > andar_atual) {
                        Controle_Subir();
                        estado_atual = ESTADO_SUBINDO;
                    } else if (andar_destino < andar_atual) {
                        Controle_Descer();
                        estado_atual = ESTADO_DESCENDO;
                    } else {
                        // Já está no andar (abre porta)
                        solicitacoes[alvo] = false; 
                        estado_atual = ESTADO_ESPERA_PORTA;
                        contador_espera = 0;
                    }
                } else {
                    // Homing: Retorna ao Térreo se ocioso
                    if (andar_atual != 0) solicitacoes[0] = true;
                }
                break;
            }
            
            case ESTADO_SUBINDO:
            case ESTADO_DESCENDO:
                // CORREÇÃO DA LÓGICA DE PARADA
                
                // 1. Prioridade Total: Cheque se CHEGOU no destino ATUAL
                if (andar_atual == andar_destino) {
                    Controle_Parar();
                    solicitacoes[andar_atual] = false; // Zera o pedido (Botão apaga)
                    estado_atual = ESTADO_ESPERA_PORTA;
                    contador_espera = 0;
                }
                // 2. Se NÃO chegou, aí sim procura carona (Otimização)
                else {
                    int novo_alvo = Buscar_Proxima_Parada();
                    if (novo_alvo != -1) {
                        andar_destino = novo_alvo; // Aceita a carona
                    }
                }
                break;

            case ESTADO_ESPERA_PORTA:
                contador_espera++;
                if (contador_espera >= 200) { // 2s
                    int proximo = Buscar_Proxima_Parada();
                    if (proximo != -1) {
                        // Lógica de reversão simplificada para proteção
                        // Sempre passa pela reversão ao sair da inércia para garantir segurança
                        estado_atual = ESTADO_REVERSAO;
                        contador_espera = 0;
                    } else {
                        estado_atual = ESTADO_PARADO;
                    }
                }
                break;

            case ESTADO_REVERSAO:
                contador_espera++;
                if (contador_espera >= 50) { // 500ms
                    estado_atual = ESTADO_PARADO; // Pronto para nova decisão
                }
                break;
        }

        // D. TELEMETRIA
        contador_telemetria++;
        if (contador_telemetria >= 30) { 
            UART_EnviaDados();
            // MAX7219_AtualizarDisplay(andar_atual, estado_motor);
            contador_telemetria = 0; 
        }

        __delay_ms(10);
    }
}