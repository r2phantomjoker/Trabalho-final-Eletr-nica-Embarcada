#include "comm.h"
#include "globals.h"    
#include "mcc_generated_files/mcc.h"

#define CR      13  //define o valor de CR na ascii

int UART_RecebePedido(char* origem_pedido, char* destino_pedido){
    if(EUSART_Read() == '$'){
    *origem_pedido = EUSART_Read();
    *destino_pedido = EUSART_Read();
    if(EUSART_Read() == CR)
        return 0;
    }
    return 1;   
    // Além do erro de não receber o <CR> ao final existe a possibilidade do <CR> ser enviado antes. Não implementei uma forma de verificar isso.
}

void UART_EnviaDados(void){
    EUSART_Write('$');
    EUSART_Write('0' + andar_atual);
    EUSART_Write('0' + andar_destino);
    EUSART_Write('0' + estado_motor);
    
    EUSART_Write('0' + (posicao_mm/100));
    EUSART_Write('0' + ((posicao_mm%100)/10));
    EUSART_Write('0' + ((posicao_mm%100)%10));
    
    EUSART_Write('0' + (velocidade_atual/100));
    EUSART_Write('0' + ((velocidade_atual%100)/10));
    EUSART_Write('.');
    EUSART_Write('0' + ((velocidade_atual%100)%10));
    
    EUSART_Write('0' + (temperatura_ponte/100));
    EUSART_Write('0' + ((temperatura_ponte%100)/10));
    EUSART_Write('.');
    EUSART_Write('0' + ((temperatura_ponte%100)%10));
    
    EUSART_Write(CR);
    return;
}

void MatrizLed (void){
    uint8_t data[2]; 
    uint8_t dig_andar = (andar_atual)<<2;
    uint8_t pos_LUT_dir = estado_atual<<2;
    CS_SetLow();
    for(uint8_t i=1;i<5;i++){
        data[0] = i;
        data[1] = LUT_Andar[dig_andar];
        SPI1_ExchangeBlock(data, 2); 
        dig_andar++;
    }
    for(uint8_t i=1;i<4;i++){
        data[0] = i;
        data[1] = LUT_dir[pos_LUT_dir];
        SPI1_ExchangeBlock(data, 2); 
        pos_LUT_dir++;
    }
    data[0] = 4;
    data[1] = pos_LUT_dir;
    data[1] = data[1] + (solicitacoes[0])*128;
    data[1] = data[1] + (solicitacoes[1])*64;
    data[1] = data[1] + (solicitacoes[2])*32;
    data[1] = data[1] + (solicitacoes[3])*16;
    SPI1_ExchangeBlock(data, 2); 
}