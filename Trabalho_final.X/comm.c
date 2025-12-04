#include "comm.h"
#include "globals.h"    
#include "mcc_generated_files/mcc.h"

#define CR      13  //define o valor de CR na ascii

int UART_RecebePedido(char* OrigemPedido, char* DestinoPedido){
    if(EUSART_Read() == '$'){
    *OrigemPedido = EUSART_Read();
    *DestinoPedido = EUSART_Read();
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