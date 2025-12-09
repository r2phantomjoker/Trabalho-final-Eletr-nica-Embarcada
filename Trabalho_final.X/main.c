/**
 * @mainpage Firmware do Elevador Inteligente
 * * @section intro_sec Introdução
 * Este projeto implementa o controle de um elevador de 4 andares utilizando o algoritmo
 * de escalonamento para otimização de percurso.
 * * @section features_sec Funcionalidades
 * - **Algoritmo: Priorização inteligente de chamadas.
 * - **Interface: Matriz de LEDs MAX7219 e Terminal Bluetooth.
 * - **Segurança: Intertravamento de motor e sensores de fim de curso.
 * - **Telemetria: Monitoramento em tempo real via UART.
 */

/**
 * @file main.c
 * @brief Lógica Avançada: SCAN, Elevador Inteligente, com Display Sincronizado.
 */

#include "mcc_generated_files/mcc.h"
#include "globals.h"
#include "comm.h"
#include "motor.h"

/**
 * @brief Código principal do sistema
 * @details Realiza a inicialização dos periféricos e gerencia o Loop Principal
 * contendo a Máquina de Estados, Leitura de Sensores e Telemetria.
 */

void main(void) {
    SYSTEM_Initialize();

    //Configurações Iniciais de Hardware
    
    // Configura o PORTB como digital 
    ANSELB = 0x00;          
    
    // Configura o pino RB1 como saída para o Chip Select (CS)
    TRISBbits.TRISB1 = 0;   
    LATBbits.LATB1 = 1;     // Inicializa em nível Alto 
    
    // Desabilita interrupções por mudança de estado
    INTCONbits.IOCIE = 0;   
    
    // Registra o callback 'SENSORES_CalcularVelocidade' no Timer 4
    TMR4_SetInterruptHandler(SENSORES_CalcularVelocidade);

    // Habilita as interrupções globais e periféricas
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    
    // Reinicia o módulo SPI para garantir sincronia
    SSP1CON1bits.SSPEN = 0; 
    SSP1CON1bits.SSPEN = 1; 

    // Garante que o motor inicie parado
    Controle_Parar(); 
    
    // Inicializa e limpa a matriz de LEDs
    //MatrizInicializa();
    
    while (1) {
        
        // A. COMUNICAÇÃO BLUETOOTH
        // Verifica se há novos dados recebidos na serial
        if(EUSART_is_rx_ready()) {
            if (UART_RecebePedido(&buffer_origem, &buffer_destino) == 0) { 
                
                // Converte caracteres ASCII para inteiros
                int origem = buffer_origem - '0';
                int destino = buffer_destino - '0';
                
                // Valida se os andares estão dentro do limite (0 a 3)
                if (origem >= 0 && origem <= 3 && destino >= 0 && destino <= 3) {
                    
                    // Atualiza a variável global de destino para telemetria
                    andar_destino = (uint8_t)destino; 
                    
                    // Define a direção da solicitação com base na origem e destino
                    if (origem < destino) { 
                        chamadas_subida[origem] = true;
                        chamadas_subida[destino] = true;
                    } 
                    else if (origem > destino) {
                        chamadas_descida[origem] = true;
                        chamadas_descida[destino] = true;
                    }
                }
            }
        }

        // B. LEITURA DE SENSORES
        // Atualiza a posição atual do elevador
        Verificar_Sensores();

        // C. MÁQUINA DE ESTADOS
        switch (estado_atual) {
            
            // Estado 1: Elevador em repouso
            case ESTADO_PARADO:
                // Prioridade 1: Atendimento local, verifica solicitações de subida no andar atual
                if (chamadas_subida[andar_atual]) {
                    Limpar_Chamada_Atual(); 
                    estado_atual = ESTADO_ESPERA_PORTA;
                    contador_espera = 0;
                }
                // Prioridade 2: Atendimento local, verifica solicitações de descida no andar atual
                else if (chamadas_descida[andar_atual]) {
                    Limpar_Chamada_Atual();
                    estado_atual = ESTADO_ESPERA_PORTA;
                    contador_espera = 0;
                }
                // Prioridade 3: Análise de chamadas pendentes nos andares superiores
                else if (Existe_Chamada_Acima(andar_atual)) {
                    Controle_Subir();
                    estado_atual = ESTADO_SUBINDO;
                }
                // Prioridade 4: Análise de chamadas pendentes nos andares inferiores
                else if (Existe_Chamada_Abaixo(andar_atual)) {
                    Controle_Descer();
                    estado_atual = ESTADO_DESCENDO;
                }
                // Prioridade 5: Retorno à base (Homing) em caso de ociosidade
                else if (andar_atual != 0) {
                    chamadas_descida[0] = true; 
                }
                break;
            
            // Estado 2: Elevador em movimento de subida     
            case ESTADO_SUBINDO:
                // Prioridade 1: Verifica se deve parar no andar atual para atendimento (Carona)
                if (chamadas_subida[andar_atual]) {
                    Controle_Parar();
                    Limpar_Chamada_Atual();
                    estado_atual = ESTADO_ESPERA_PORTA;
                    contador_espera = 0;
                }
                // Prioridade 2: Verifica o fim do percurso de subida
                else if (!Existe_Chamada_Acima(andar_atual)) {
                    
                    // Se houver requisição de descida neste andar, realiza a inversão de serviço
                    if (chamadas_descida[andar_atual]) {
                        Controle_Parar();
                        Limpar_Chamada_Atual();
                        estado_atual = ESTADO_ESPERA_PORTA;
                        contador_espera = 0;
                    } 
                    // Se não houver mais solicitações, retorna ao repouso
                    else {
                        Controle_Parar();
                        estado_atual = ESTADO_PARADO;
                    }
                }
                break;
            
            // Estado 3: Elevador em movimento de descida     
            case ESTADO_DESCENDO:
                // Prioridade 1: Verifica se deve parar no andar atual para atendimento
                if (chamadas_descida[andar_atual]) {
                    Controle_Parar();
                    Limpar_Chamada_Atual();
                    estado_atual = ESTADO_ESPERA_PORTA;
                    contador_espera = 0;
                }
                // Prioridade 2: Verifica o fim do percurso de descida
                else if (!Existe_Chamada_Abaixo(andar_atual)) {
                    
                    // Se houver requisição de subida neste andar, realiza a inversão de serviço
                    if (chamadas_subida[andar_atual]) {
                         Controle_Parar();
                         Limpar_Chamada_Atual();
                         estado_atual = ESTADO_ESPERA_PORTA;
                         contador_espera = 0;
                    } 
                    // Se não houver mais solicitações, retorna ao repouso
                    else {
                        Controle_Parar();
                        estado_atual = ESTADO_PARADO;
                    }
                }
                break;
            
            // Estado 4: Simulação de porta aberta - Tempo de embarque
            case ESTADO_ESPERA_PORTA:
                contador_espera++;
                // Temporização: 200 ciclos * 10ms = 2 Segundos
                if (contador_espera >= 200) { 
                    estado_atual = ESTADO_REVERSAO;
                    contador_espera = 0;
                }
                break;
            
            // Estado 5: Reversão de segurança 
            case ESTADO_REVERSAO:
                contador_espera++;
                // Temporização: 50 ciclos * 10ms = 0.5 Segundos
                // Garante a parada total do motor antes de nova manobra
                if (contador_espera >= 50) { 
                    estado_atual = ESTADO_PARADO; 
                }
                break;
        }

        // D. TELEMETRIA E INTERFACE 
        contador_telemetria++;

        // Verifica se passaram 300ms 
        if (contador_telemetria >= 30) { 
            
            // Envia os dados de telemetria via UART
            UART_EnviaDados();
            
            // Mapeamento de Dados: Unifica vetores de subida/descida para visualização única na Matriz
            for(int i=0; i<4; i++) {
                solicitacoes[i] = chamadas_subida[i] || chamadas_descida[i];
            }
            
            // Atualiza o display da Matriz de LEDs
            //MatrizLed();
            
            // Reinicia o contador de tempo
            contador_telemetria = 0; 
        }

        __delay_ms(10);
    }
}