/**
 * @file comm.c
 * @brief Implementação da comunicação UART e Driver MAX7219 (Corrigido)
 */

#include "comm.h"
#include "globals.h"    
#include "mcc_generated_files/mcc.h"

#define CR      13 


// ==========================================
// TABELAS DE DADOS (LUTs)
// ==========================================

const uint8_t LUT_Andar[]= {
    0b00000000, //1
    0b00000001,
    0b11111111, 
    0b01000001,

    0b01110001, //2
    0b10001001,
    0b10000101,
    0b01000011,
    
    0b01101110, //3 
    0b10010001, 
    0b10000001, 
    0b01000010, 

    0b11111111, //4
    0b00010000, 
    0b00010000, 
    0b11110000
};

const uint8_t LUT_dir[] = {
    0b00000000, //Parado //0
    0b00000000, //1
    0b00000000, //2
    0b00000000, //3
           
    0b00000000, //Subindo
    0b00000010,
    0b00000100,
    0b00000010,
    
    0b00000000, //Descendo
    0b00000010,
    0b00000001,
    0b00000010,
    
    0b00000000, //Esperar porta
    0b00000010,
    0b00000010,
    0b00000010,
    
    0b00000000, //Reversão
    0b00000010,
    0b00000010,
    0b00000010
};

const uint8_t LUT_percurso[] ={
    0b00010000, 
    0b00100000,
    0b01000000,
    0b10000000
};

const uint8_t matrix_conf[] = {
    0x09,0x00,  // Decode mode = 0
    0x0A,0x00,  // Intensity 1/32
    0x0B,0x07,  // Scan Limit
    0x0C,0x01,  // Shutdown mode = 1
    0x0F,0x01,  // Display-Test = 1
    0x0F,0x00,  // Display-Test = 0
};

// ==========================================
// FUNÇÕES AUXILIARES (PRIVADAS)
// ==========================================

/**
 * @brief Envia um comando de 16 bits para o MAX7219 respeitando o protocolo.
 * Protocolo: CS Low -> Envia Endereço -> Envia Dado -> CS High (Latch)
 */
void MAX7219_Write(uint8_t address, uint8_t data) {
    CS_SetLow(); // Seleciona o chip
    
    // 1. Envia Endereço (Registro)
    SSP1BUF = address;
    while(!PIR1bits.SSP1IF); // Espera enviar
    PIR1bits.SSP1IF = 0;     // Limpa flag
    
    // 2. Envia Dado (Valor)
    SSP1BUF = data;
    while(!PIR1bits.SSP1IF); // Espera enviar
    PIR1bits.SSP1IF = 0;     // Limpa flag
    
    CS_SetHigh(); // LATCH: Salva o comando e mostra no LED
}


// ==========================================
// FUNÇÕES UART (MANTIDAS DO ORIGINAL)
// ==========================================

int UART_RecebePedido(char* origem_pedido, char* destino_pedido){
    if(EUSART_Read() == '$'){
        *origem_pedido = EUSART_Read();
        *destino_pedido = EUSART_Read();
        // Pequena proteção: Se não vier CR, retorna erro, mas consome o buffer
        if(EUSART_Read() == CR)
            return 0;
    }
    return 1;   
}

void UART_EnviaDados(void){
    EUSART_Write('$');
    EUSART_Write('0' + andar_atual);
    EUSART_Write('0' + andar_destino);
    EUSART_Write('0' + estado_motor);
    EUSART_Write(posicao_mm);
    EUSART_Write(velocidade_atual);
    EUSART_Write(temperatura_ponte);
    
//    EUSART_Write('0' + (posicao_mm/100));
//    EUSART_Write('0' + ((posicao_mm%100)/10));
//    EUSART_Write('0' + ((posicao_mm%100)%10));
//    
//    EUSART_Write('0' + (velocidade_atual/100));
//    EUSART_Write('0' + ((velocidade_atual%100)/10));
//    EUSART_Write('.');
//    EUSART_Write('0' + ((velocidade_atual%100)%10));
//    
//    EUSART_Write('0' + (temperatura_ponte/100));
//    EUSART_Write('0' + ((temperatura_ponte%100)/10));
//    EUSART_Write('.');
//    EUSART_Write('0' + ((temperatura_ponte%100)%10));
    
    EUSART_Write(CR);
    return;
}

// ==========================================
// FUNÇÕES DA MATRIZ (CORRIGIDAS)
// ==========================================

void MatrizInicializa(void){
    // Garante que a SPI não vai gerar interrupção (redundância de segurança)
    PIE1bits.SSP1IE = 0; 
    
    CS_SetHigh(); // Estado inicial seguro
    
    // Loop para enviar as configurações definidas no vetor matrix_conf
    // O vetor tem pares: [Endereço, Valor, Endereço, Valor...]
    // Tamanho do vetor matrix_conf original = 12 bytes (6 pares)
    uint8_t i = 0;
    while(i < 12) {
        MAX7219_Write(matrix_conf[i], matrix_conf[i+1]);
        i += 2; // Pula de par em par
    }
    
    // Limpa a tela (Desliga todos os LEDs inicialmente)
    for(uint8_t row=1; row<=8; row++){
        MAX7219_Write(row, 0x00);
    }
}


int Uniaomatrix (void){
    
}


void MatrizLed (void){
    // Lógica simplificada:
    // Linhas 1 a 4: Mostram o número do andar (LUT_Andar)
    // Linhas 5 a 8: Mostram a seta de direção (LUT_dir)
    
    // Recalcula índices para pegar o desenho certo
    // Nota: Supondo que seus desenhos nas LUTs tenham 4 bytes de altura cada
    uint8_t buffer_percurso = 0;
    uint8_t base_andar = andar_atual * 4;   // 0->0, 1->4, 2->8...
    uint8_t base_seta  = estado_atual * 4;  // 0->0, 1->4...

    // Desenha o Número do Andar nas linhas 1, 2, 3, 4
    for(uint8_t i=0; i<4; i++){
        // Linha do MAX (1 a 4), Dado da LUT
        MAX7219_Write(i+1, LUT_Andar[base_andar + i]);
    }

    // Desenha a Seta nas linhas 5, 6, 7, 8
    // Nota: Ajustei para usar as linhas de cima da matriz para a seta
    
    for(uint8_t i=0; i<4; i++){
        // Linha do MAX (5 a 8), Dado da LUT
        MAX7219_Write(i+5, LUT_dir[base_seta + i]); 
    }
    
    buffer_percurso = LUT_dir[base_seta + 3];
        for (uint8_t i=0; i<4; i++){
            if(solicitacoes[i]){
                buffer_percurso += LUT_percurso[i] ;
            }
        }
    MAX7219_Write(8, buffer_percurso); 
    // OBS: Removi a parte dos "pontinhos" das solicitações na linha 4
    // para simplificar e garantir que o básico funcione primeiro.
    // Se quiser adicionar de volta, tem que fazer uma lógica de "Bitwise OR" (|)
    // antes de enviar o comando da linha específica.
}