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



// VARIÁVEIS DE CONTROLE
uint16_t contador_telemetria = 0;
uint16_t contador_espera = 0;
char buffer_origem, buffer_destino;


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
                int origem = buffer_origem - '0';
                int destino = buffer_destino - '0';
                if (origem >= 0 && origem <= 3) solicitacoes[origem] = true;
                if (destino >= 0 && destino <= 3) solicitacoes[destino] = true;
            }
        }

        // B. SENSORES
        Verificar_Sensores();

        // C. MÁQUINA DE ESTADOS
        switch (estado_atual) {
            case ESTADO_PARADO: {
                int alvo = Buscar_Proxima_Parada();
                if (alvo != -1) {
                    andar_destino = (uint8_t)alvo;
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
                        andar_destino = (uint8_t)novo_alvo; // Aceita a carona
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
            MatrizLed();
            contador_telemetria = 0; 
        }

        __delay_ms(10);
    }
}