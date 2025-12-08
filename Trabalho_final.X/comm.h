/**
 * @file comm.h
 * @brief Cabeçalho da comunicação UART e Interface de Display.
 */

#ifndef COMM_H
#define	COMM_H

#include <stdint.h>

/*
 * CONSTANTES E TABELAS
 */

/**
 * @brief LUT para os desenhos dos numeros 
 */
extern const uint8_t LUT_Andar[];

/**
 * @brief LUT para os desenhos dos status do elevador
 */
extern const uint8_t LUT_dir[];

/**
 * @brief Tabela contendo a sequência de comandos de inicialização do MAX7219.
 */
extern const uint8_t matrix_conf[];


/*
 * PROTÓTIPOS DE FUNÇÕES
 */

/**
 * @brief Verifica o buffer da UART em busca de um pedido válido.
 * @note Protocolo esperado: '$' + Origem + Destino + CR.
 * * @param OrigemPedido -  Ponteiro onde será salvo o caractere da origem.
 * @param DestinoPedido - Ponteiro onde será salvo o caractere do destino.
 * * @return 0 - Sucesso, mensagem válida.
 * @return 1 - Erro, mensagem incompleta.
 */
int UART_RecebePedido(char* OrigemPedido, char* DestinoPedido);

/**
 * @brief Coleta os estados globais do sistema e envia via telemetria.
 * @note Envia: Andar atual, destino, motor, posição, velocidade e temperatura.
 * Formato CSV iniciado por '$' e finalizado por CR.
 */
void UART_EnviaDados(void);

/**
 * @brief Atualiza a Matriz de LEDs com base no estado atual.
 * @details Renderiza o número do andar, a seta de direção
 * e sobrepõe as solicitações de andares na linha inferior.
 */
void MatrizLed (void);

/**
 * @brief Inicializa o hardware da Matriz de LEDs.
 * @details Configura a SPI, envia os comandos de setup para o MAX7219
 * e limpa a tela.
 */
void MatrizInicializa(void);

#endif