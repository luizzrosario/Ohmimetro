# EmbarcOhm

### Desenvolvido por: Luiz Rosário  
**Polo:** Ilhéus/Itabuna  
**Data:** 27/04/2025

---

## Descrição do Projeto

O **EmbarcOhm** é um ohmímetro baseado na plataforma **BitDogLab RP2040**, capaz de calcular a resistência de um resistor desconhecido utilizando um resistor de referência conhecido. O projeto também exibe o valor encontrado em um display OLED e representa as cores das faixas do resistor por meio de uma matriz de LEDs WS2812.

A medição é baseada na leitura de tensão pelo ADC (Conversor Analógico-Digital) e utiliza a fórmula:

```
R_x = (R_conhecido * média_ADC) / (ADC_RESOLUTION - média_ADC)
```

Além disso, ele aproxima o valor real ao resistor comercial mais próximo (baseado na tabela E24).

---

## Objetivo Geral

Construir um **ohmímetro funcional** que:

- Leia e calcule a resistência de um resistor desconhecido.
- Mostre no display OLED:
  - O valor real e comercial da resistência.
  - As cores correspondentes às faixas do resistor.
- Utilize uma matriz de LEDs WS2812 para representar visualmente essas faixas.

---

## Funcionalidades

- **Medição de Resistência**: utilizando um resistor de 10kΩ como referência.
- **Display OLED**:
  - Valor medido de resistência.
  - Valor de ADC médio.
  - Faixas de cores do resistor.
- **Matriz de LEDs WS2812**:
  - Exibição visual das faixas de cor do resistor medido.
- **Reset para BOOTSEL Mode**: através do botão B conectado ao GPIO 6.

---

## Tecnologias Utilizadas

- **BitDogLab RP2040**
- **C (Pico SDK)**
- **Periféricos Utilizados**:
  - ADC (Conversor Analógico-Digital)
  - I2C (para o display OLED SSD1306)
  - PIO (Programação de LEDs WS2812)
  - GPIO (botões de controle)

---

## Componentes e Materiais

- Placa BitDogLab RP2040
- Display OLED SSD1306 (via I2C)
- Matriz de LEDs WS2812 (NeoPixel)
- Resistores diversos para testes
- Resistor de 10kΩ (referência)
- Botões para interação
- Cabos e protoboard

---

## Pontos de Atenção

- **Precisão**: Devido ao uso de componentes simples e falta de calibração refinada, pequenas imprecisões podem ocorrer, especialmente para resistores de baixo valor.
- **Faixas de Cores**: As cores exibidas nos LEDs podem não corresponder 100% às cores padrão devido às limitações de cores disponíveis na matriz WS2812.
- **Tensão de Referência do ADC**: Foi utilizada uma referência de 3.31V, calibrada conforme a alimentação disponível no setup.

---

## Como Funciona

1. O ADC lê a tensão no divisor resistivo (resistor conhecido + resistor desconhecido).
2. Calcula-se a média de 500 leituras para minimizar ruído.
3. A fórmula simplificada é aplicada para encontrar o valor de resistência do resistor desconhecido.
4. O valor é arredondado para o resistor comercial mais próximo (padrão E24).
5. O valor e as faixas de cores são exibidos no display OLED e nos LEDs.

---

## Estrutura do Código

- Inicialização de periféricos: ADC, I2C, PIO.
- Funções para:
  - Calcular resistência.
  - Encontrar o resistor comercial mais próximo.
  - Mostrar faixas de cores no display e nos LEDs.
- Loop principal:
  - Amostragem do ADC.
  - Cálculo da resistência.
  - Atualização da tela OLED.
  - Atualização da matriz de LEDs.

---

## Links

- **Código-Fonte**: [GitHub] <https://github.com/luizzrosario/Ohmimetro> 
- **Vídeo de Demonstração**: [YouTube] <https://www.youtube.com/watch?v=RHq0hsYkM5w>
