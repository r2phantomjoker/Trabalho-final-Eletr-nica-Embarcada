# 🚧 Controle de Elevador 4 Andares por Bluetooth

Projeto acadêmico desenvolvido para a disciplina **Eletrônica Embarcada (FGA0096)** da Universidade de Brasília.  
O sistema simula o funcionamento de um elevador real utilizando microcontrolador PIC, controle de motor DC, sensores, comunicação Bluetooth e interface visual com matriz de LEDs.

---

## 📌 Objetivo

Integrar os conteúdos aprendidos na disciplina por meio da criação de um sistema embarcado funcional.  
O elevador deve atender solicitações de andares, otimizar seu percurso e apresentar informações ao usuário via Bluetooth e display LED.

---

## 🧠 Funcionalidades Principais

- Controle do motor DC por **PWM (10 bits)** + **direção digital**.
- Gerenciamento de movimento com **ponte H TB6612FNG**.
- **Sensores magnéticos Hall (S1–S4)** para detectar os andares.
- **Encoder óptico** para medir posição (mm) e velocidade (mm/s).
- Monitoramento da **temperatura da ponte H** com sensor LM35 (ADC 10 bits).
- Interface visual com **matriz de LEDs MAX7219 via SPI (1 MHz)**.
- Comunicação Bluetooth via **HC-06 UART (ASCII 19200bps)**.
- Sistema de atendimento de **até 5 solicitações simultâneas** (origem → destino).

---

## 🔌 Hardware Utilizado

- **PIC16F1827 (5V)**
- **Motor CC com engrenagem**
- **Ponte H TB6612FNG**
- **Sensores de efeito Hall A3144**
- **Encoder óptico + disco**
- **Matriz LED 8x8 com MAX7219**
- **Sensor LM35**
- **Bluetooth HC-06**

---

## 📡 Comunicação Bluetooth

O microcontrolador transmite um pacote a cada **300 ms** no formato ASCII:

Exemplo:
**Campos:**
- **A** → último andar atingido (0–3)
- **D** → destino solicitado
- **M** → estado do motor  
  - 0 = parado  
  - 1 = subindo  
  - 2 = descendo
- **HHH** → posição (0–180 mm)
- **VV.V** → velocidade (mm/s, 1 casa decimal)
- **TT.T** → temperatura da ponte H (°C)


## ▶️ Algoritmo de Atendimento

- São processadas **até 5 solicitações**.
- O elevador atende **todas as paradas no mesmo sentido** antes de inverter.
- Cada chamada possui **origem e destino**, que devem ser respeitados.
- Quando não há solicitações, o sistema retorna para **andar 0 (repouso)**.
- Proteções:
  - **2s** de parada para embarque/desembarque
  - **500ms** antes de inverter a direção

---

## 🖥️ Interface na Matriz de LEDs (MAX7219)

- Colunas 1–4: último andar atingido.
- Colunas superiores 5–7: seta indicando **direção do movimento**.
- Colunas inferiores 5–7: pontos representando os **andares pendentes**.

Essa visualização facilita o monitoramento do sistema em tempo real.
