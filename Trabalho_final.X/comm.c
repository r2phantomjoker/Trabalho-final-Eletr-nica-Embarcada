/**
 * @file comm.c
 * @brief Implementação da comunicação UART e Driver MAX7219 
 */

#include "comm.h"
#include "globals.h"    
#include "mcc_generated_files/mcc.h"

/**
 * @brief Definição do caractere ASCII para "Carriage Return".
 * Utilizado como fim da mensagem no protocolo UART.
 */
#define CR      13 


// TABELAS DE DADOS (LUTs)


/**
 * @brief Tabela de padrões de bits para desenhar os números dos andares.
 * @note Estrutura: Cada número é formado por 4 bytes
 * - Índice 0-3:   Andar 1 
 * - Índice 4-7:   Andar 2
 * - Índice 8-11:  Andar 3
 * - Índice 12-15: Andar 4
 */
const uint8_t LUT_Andar[]= {
    // Andar 1
    0b00000000, 
    0b00000001,
    0b11111111, 
    0b01000001,

    // Andar 2
    0b01110001, 
    0b10001001,
    0b10000101,
    0b01000011,
    
    // Andar 3 
    0b01101110, 
    0b10010001, 
    0b10000001, 
    0b01000010, 

    // Andar 4
    0b11111111, 
    0b00010000, 
    0b00010000, 
    0b11110000
};

/**
 * @brief Tabela de padrões de bits para indicar o status do elevador.
 * @note Cada estado consome 4 bytes da memória.
 * Estados representados na matriz:
 * - Parado:       Nenhum LED aceso
 * - Subindo:      Seta apontando para cima
 * - Descendo:     Seta apontando para baixo
 * - Esperar/Rev:  Seta horizontal
 */
const uint8_t LUT_dir[] = {
    // 0: Parado
    0b00000000, 
    0b00000000, 
    0b00000000, 
    0b00000000, 
            
    // 1: Subindo
    0b00000000, 
    0b00000010,
    0b00000100,
    0b00000010,
    
    // 2: Descendo
    0b00000000, 
    0b00000010,
    0b00000001,
    0b00000010,
    
    // 3: Esperar porta
    0b00000000, 
    0b00000010,
    0b00000010,
    0b00000010,
    
    // 4: Reversão
    0b00000000, 
    0b00000010,
    0b00000010,
    0b00000010
};

/**
 * @brief Máscara de bits para indicar andares solicitados na rota.
 * @note Utilizado para acender um LED específico correspondente ao andar na rota.
 */
const uint8_t LUT_percurso[] = {
    0b00010000, // Andar 1
    0b00100000, // Andar 2
    0b01000000, // Andar 3
    0b10000000  // Andar 4
};

/**
 * @brief Tabela de inicialização e configuração do driver MAX7219.
 * * @note Estrutura do vetor: Pares de bytes ordenados [Endereço do Registrador, Dado].
 * A função MatrizInicializa() percorre este vetor de 2 em 2 bytes.
 * * @details Configurações aplicadas:
 * - 0x09 (Decode Mode): 0x00 - Controle individual dos LEDs (Matriz).
 * - 0x0A (Intensity):   0x00 - Brilho mínimo.
 * - 0x0B (Scan Limit):  0x07 - Habilita todas as 8 linhas/dígitos.
 * - 0x0C (Shutdown):    0x01 - Sai do modo de economia de energia.
 * - 0x0F (Display Test):0x01 - Pisca a tela para reset visual.
 */
const uint8_t matrix_conf[] = {
    0x09, 0x00,  // Decode mode: No decode (para matriz de pontos)
    0x0A, 0x00,  // Intensity: Brilho mínimo 
    0x0B, 0x07,  // Scan Limit: Usa todos os 8 dígitos, linhas
    0x0C, 0x01,  // Shutdown: Modo de operação normal
    0x0F, 0x01,  // Display-Test: Liga todos os LEDs
    0x0F, 0x00,  // Display-Test: Retorna ao funcionamento normal
};



// FUNÇÕES UART



/**
 * @brief Tenta receber e decodificar um pacote de pedido via UART.
 * @note Protocolo esperado: "$OD\r" 
 * - Onde: '$' = Início, O = Origem, D = Destino, \r = CR/13
 * * @param origem_pedido  Ponteiro para armazenar o andar de origem do elevador.
 * @param destino_pedido Ponteiro para armazenar o andar de destino.
 * * @return 0 - Pacote recebido e validado com o terminador correto.
 * @return 1 - Cabeçalho não encontrado ou pacote incompleto/corrompido.
 */
int UART_RecebePedido(char* origem_pedido, char* destino_pedido){
    
    // 1. Verifica o Cabeçalho 
    if(EUSART_Read() == '$'){
        
        // 2. Extração de Dados
        // Como são passados variáveis por referência, é utilizado o operador '*'
        // para escrever o valor lido diretamente na variável original da main
        *origem_pedido = EUSART_Read();
        *destino_pedido = EUSART_Read();
        
        // 3. Validação do Terminador
        // O protocolo exige que a mensagem termine com CR
        if(EUSART_Read() == CR)
            return 0; // Retorna 0 indicando sucesso na validação total
    }
    
    // Se chegar aqui, indica falha e retorna 1
    return 1;    
}

/**
 * @brief Transmite o pacote de telemetria do sistema via UART.
 * @note Protocolo do Pacote: "$A,D,M,PPP,VV.V,TT.T\r"
 * Onde:
 * - $: Cabeçalho de início de frame.
 * - A: Andar Atual (0-9)
 * - D: Andar Destino (0-9)
 * - M: Estado do Motor (0-9)
 * - PPP: Posição em mm (000-999)
 * - VV.V: Velocidade (00.0-99.9)
 * - TT.T: Temperatura (00.0-99.9)
 * - \r: Terminador (Carriage Return)
 */
void UART_EnviaDados(void){
    
    // 1. Cabeçalho
    // Indica para o receptor que uma nova mensagem iniciou
    EUSART_Write('$');

    // 2. Andar Atual 
    // Soma '0' para converter o valor numérico (0-9) no char ASCII correspondente
    EUSART_Write('0' + andar_atual);
    EUSART_Write(','); 

    // 3. Andar Destino 
    EUSART_Write('0' + andar_destino);
    EUSART_Write(',');

    // 4. Estado Motor 
    EUSART_Write('0' + estado_motor);
    EUSART_Write(',');

    // 5. Posição
    EUSART_Write('0' + (posicao_mm / 100));        // Centena
    EUSART_Write('0' + ((posicao_mm % 100) / 10)); // Dezena
    EUSART_Write('0' + (posicao_mm % 10));         // Unidade
    EUSART_Write(',');

    // 6. Velocidade
    EUSART_Write('0' + (velocidade_atual / 100));        // Centena
    EUSART_Write('0' + ((velocidade_atual % 100) / 10)); // Dezena
    EUSART_Write('.');                                   // Insere o ponto decimal manualmente
    EUSART_Write('0' + (velocidade_atual % 10));         // Unidade (decimal)
    EUSART_Write(',');

    // 7. Temperatura 
    EUSART_Write('0' + (temperatura_ponte / 100));
    EUSART_Write('0' + ((temperatura_ponte % 100) / 10));
    EUSART_Write('.'); 
    EUSART_Write('0' + (temperatura_ponte % 10));

    // 8. Finalizador de Linha 
    // Envia o CR para indicar o fim do pacote
    EUSART_Write(13); 
}


// FUNÇÕES DA MATRIZ 


/**
 * @brief Envia um pacote de 16 bits para o driver MAX7219 via SPI.
 * @note O MAX7219 exige a seguinte sequência rigorosa:
 * 1. CS Low, Habilita comunicação.
 * 2. Envia 8 bits de Endereço, MSB.
 * 3. Envia 8 bits de Dados, LSB.
 * 4. CS High (Borda de subida carrega os dados no registrador interno - Latch).
 * * @param address Endereço do registrador do MAX7219.
 * @param data Valor a ser escrito no registrador.
 */
void MAX7219_Write(uint8_t address, uint8_t data) {
    
    // 1. Inicia a transação SPI
    // O pino LOAD/CS do MAX7219 deve ir para 0 para começar a receber bits
    CS_SetLow(); 
    
    // 2. Transmite o Endereço 
    SSP1BUF = address;           // Escrever no buffer, inicia o Clock do hardware
    while(!PIR1bits.SSP1IF);     // Espera o hardware enviar todos os 8 bits
    PIR1bits.SSP1IF = 0;         // Limpa a flag
    
    // 3. Transmite o Dado
    SSP1BUF = data;              // Inicia o envio do segundo byte
    while(!PIR1bits.SSP1IF);     // Espera o hardware enviar todos os 8 bits
    PIR1bits.SSP1IF = 0;         // Limpa a flag
    
    // 4. Finaliza e Salva
    CS_SetHigh(); 
}


/**
 * @brief Inicializa o driver MAX7219 e prepara a Matriz de LEDs.
 */
void MatrizInicializa(void){
    
    // 1. Desabilita Interrupção SPI
    PIE1bits.SSP1IE = 0; 
    
    // 2. Estado Inicial do Chip Select
    CS_SetHigh(); 
    
    // 3. Carrega Configurações
    uint8_t i = 0;
    while(i < 12) {
        MAX7219_Write(matrix_conf[i], matrix_conf[i+1]);
        i += 2;
    }
    
    // 4. Limpa a Tela
    for(uint8_t i=1; i<=8; i++){ 
        MAX7219_Write(i, 0x00);
    }
}



/**
 * @brief Atualiza o conteúdo visual da Matriz de LEDs 
 * @details A tela é dividida logicamente em duas áreas:
 * - Parte Superior (Linhas 1-4): Exibe o número do andar atual.
 * - Parte Inferior (Linhas 5-8): Exibe a seta de direção e status.
 */
void MatrizLed (void){
    
    // 1. Cálculo dos Índices
    // Multiplicamos por 4 para encontrar o bloco de dados correto no vetor
    uint8_t base_andar = andar_atual * 4;   
    uint8_t base_seta  = estado_atual * 4;  

    // 2. Renderiza a Parte Superior 
    for(uint8_t i=0; i<4; i++){
        MAX7219_Write(i+1, LUT_Andar[base_andar + i]);
    }

    // 3. Renderiza a Parte Inferior
    for(uint8_t i=0; i<4; i++){
        MAX7219_Write(i+5, LUT_dir[base_seta + i]); 
    }
    
    // 4. Lógica de Sobreposição 
    // Recupera o desenho original da base da seta 
    uint8_t buffer_percurso = LUT_dir[base_seta + 3];
    
    // Verifica vetor de solicitações e adiciona os bits correspondentes
    for (uint8_t i=0; i<4; i++){
        if(solicitacoes[i]){
            // Soma do bit da solicitação ao desenho da seta
            buffer_percurso += LUT_percurso[i]; 
        }
    }
    
    // 5. Atualiza a Linha 8 com a imagem fundida
    MAX7219_Write(8, buffer_percurso); 
}