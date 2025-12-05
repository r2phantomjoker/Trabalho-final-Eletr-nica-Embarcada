# Elevador GUI

**O que faz**
- Lista **portas**, permite **selecionar** e **Conectar/Desconectar**.
- Recebe o quadro `$A,D,M,HHH,VV.V,TT.T\r` a 19200 bps, 8N1, CR.
- Envia solicitações `$OD\r` (O,D ∈ 0..3).
- Plota **Posição**, **Velocidade** e **Temperatura** em tempo real (altura dos gráficos ajustada para melhor legibilidade).
- Grava CSV opcionalmente.

## Instalação
```bash
pip install pyserial matplotlib
```

## Uso
```bash
python elevador.py
```

1. Clique em **Atualizar** e escolha a **porta**.
2. Clique **Conectar** (ou **Desconectar**).
3. Use `$OD\r` para solicitar percurso e observe os gráficos.

## Pareamento do HC‑05 / HC‑06
Antes de conectar no aplicativo:
1. Pareie o módulo **HC‑05 ou HC‑06** com o computador pelo **Bluetooth** do Windows.  
   - Vá em *Configurações → Bluetooth e dispositivos → Adicionar dispositivo → Bluetooth*.
   - Escolha o módulo (geralmente “HC‑05” ou “HC‑06”).  
   - Digite o **PIN 1234** (ou 0000 se configurado assim).  
2. Após o pareamento, o Windows criará **portas COM virtuais** (ex.: `COM21`, `COM22`).  
3. Essas portas aparecerão na lista do programa e uma delas deve ser usadas para conectar (testar qual delas conecta).

## Notas
- A listagem filtra pelo prefixo `COM` em Windows (ex.: `COM3`). Se nada aparecer, verifique o driver ou o pareamento.
- Ajuste `MAX_POINTS` e `PLOT_INTERVAL_MS` conforme a taxa de atualização desejada.
- O algoritmo de controle/filas fica no firmware; o app apenas envia `$OD` e exibe dados.

Licença: MIT
