/**
 * @file comm.h
 * @author Gabriel Celestino
 * @brief Cabeçalho da comunicação e  interface.
 * @date Created on November 30, 2025
 */

#ifndef COMM_H
#define	COMM_H

#include <stdint.h>

const uint8_t LUT_Andar[]= {
    0b00000000, //1
    0b10000010,
    0b11111111, 
    0b10000000,

    0b11000010, //2 
    0b10100001,
    0b10010001, 
    0b10001110, 

    0b01000010, //3 
    0b10000001, 
    0b10001001, 
    0b01110110, 

    0b00000111, //4
    0b00000100, 
    0b00000100, 
    0b11111111
};

const uint8_t LUT_dir[] = {
    0b00000000, //Parado
    0b00000000,
    0b00000000,
    0b00000000,
            
    0b00000000, //Subindo
    0b00000010,
    0b00000001,
    0b00000010,
    
    0b00000000, //Descendo
    0b00000010,
    0b00000100,
    0b00000010
};

/**
 * @brief Recebe e salva os dados da UART no endereço designado.
 * Retorna 0 caso último caractere recebido seja  <CR>.
 * Retorna 1 caso último caractere recebido seja diferente de <CR>.
 * @note Os dados não recebem tratamento aqui.
 */
int UART_RecebePedido(char* OrigemPedido, char* DestinoPedido);

/*
 * @brief Acessa as variaveis globais e transmite pela UART
 * Os valores já são convertidos dentro da transmissão sem interferir nos valores.
 */
void UART_EnviaDados(void);

/*
 * Pega os valores das variaveis globais e atualiza a matriz de LEDs
 */
void MatrizLed (void);
#endif	/* COMM_H */

