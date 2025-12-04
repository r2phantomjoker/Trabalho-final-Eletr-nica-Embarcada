/**
 * @file main.c
 * @brief Teste de Integração: Motor, Odometria (TMR0) e Lógica de Controle.
 */

#include "mcc_generated_files/mcc.h"
#include "motor.h"
#include "globals.h"
#include "comm.h"

void main(void)
{
    // 1. INICIALIZAÇÃO DO SISTEMA
    SYSTEM_Initialize();

    // 2. CONEXÃO DO VELOCÍMETRO (TIMER 4)
    // Registra a função que lê o hardware TMR0 e calcula a física a cada 100ms.
    TMR4_SetInterruptHandler(SENSORES_CalcularVelocidade);

    // 3. HABILITA INTERRUPÇÕES (Crucial para o Timer 4 funcionar)
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    
    MOTOR_reset(); 

    

    while (1)
    {

    }
}