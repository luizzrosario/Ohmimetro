/*
 * Por: Wilton Lacerda Silva
 *    Ohmímetro utilizando o ADC da BitDogLab
 *
 * 
 * Neste exemplo, utilizamos o ADC do RP2040 para medir a resistência de um resistor
 * desconhecido, utilizando um divisor de tensão com dois resistores.
 * O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A
#define TAMANHO_E24 (sizeof(resistores_E24) / sizeof(resistores_E24[0]))

int R_conhecido = 10000;   // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)
const float resistores_E24[] = {
  510, 560, 620, 680, 750, 820, 910,
  1000, 1100, 1200, 1300, 1500, 1600, 1800, 2000,
  2200, 2400, 2700, 3000, 3300, 3600, 3900, 4300,
  4700, 5100, 5600, 6200, 6800, 7500, 8200, 9100,
  10000, 11000, 12000, 13000, 15000, 16000, 18000,
  20000, 22000, 24000, 27000, 30000, 33000, 36000,
  39000, 43000, 47000, 51000, 56000, 62000, 68000,
  75000, 82000, 91000, 100000
};

float encontrarValorMaisProximo(float resistor) {
  float maisProximo = resistores_E24[0];
  float menorDiferenca = fabsf(resistor - resistores_E24[0]);
  
  for (int i = 1; i < TAMANHO_E24; i++) {
      float diferencaAtual = fabsf(resistor - resistores_E24[i]);
      if (diferencaAtual < menorDiferenca) {
          menorDiferenca = diferencaAtual;
          maisProximo = resistores_E24[i];
      }
  }
  
  return maisProximo;
}

const char* cores[] = {
  "PRE",  // 0 - Preto
  "MAR",  // 1 - Marrom
  "VER",  // 2 - Vermelho
  "LAR",  // 3 - Laranja
  "AMA",  // 4 - Amarelo
  "VRD",  // 5 - Verde
  "AZU",  // 6 - Azul
  "VIO",  // 7 - Violeta
  "CIN",  // 8 - Cinza
  "BRA",  // 9 - Branco
  "DOU",  // 10 - Dourado
  "PRA"   // 11 - Prata
};

ssd1306_t ssd; // Variável para o display OLED (antes de declarar a função para não dar erro de compilação)

// Função para desenhar as cores no OLED
void mostrarCoresResistor(float valor) {
  float valorProximo = encontrarValorMaisProximo(valor);
  int digito1, digito2, multiplicador = 0;
  int valorInt = (int)valorProximo;

  // Normalizar o valor para pegar dois dígitos
  while (valorInt >= 100) {
    valorInt /= 10;
    multiplicador++;
  }

  digito1 = valorInt / 10;
  digito2 = valorInt % 10;

  // Exibir no display
  ssd1306_draw_string(&ssd, cores[digito1], 23, 20);   // Primeira faixa
  ssd1306_draw_string(&ssd, cores[digito2], 53, 20);  // Segunda faixa
  ssd1306_draw_string(&ssd, cores[multiplicador], 83, 20); // Multiplicador

  // Opcional: Poderia mostrar a tolerância aqui se quiser (normalmente Marrom ou Dourado)
  // ssd1306_draw_string(&ssd, "DOU", 98, 16);  // Exemplo de tolerância 5%
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

int main()
{
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  float tensao;
  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string
  char str_Cor[5]; // Buffer para armazenar a string

  bool cor = true;
  while (true)
  {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

      // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
      R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);

    sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
    sprintf(str_y, "%1.0f", R_x);   // Converte o float em string

    sprintf(str_Cor, "%1.0f", encontrarValorMaisProximo(R_x)); 

    // cor = !cor;
    //  Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);                          // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
    ssd1306_draw_string(&ssd, str_Cor, 8, 6); // Desenha uma string
    mostrarCoresResistor(encontrarValorMaisProximo(R_x)); // Chama a função para mostrar as cores);
    // ssd1306_draw_string(&ssd, "Cor", 8, 16);  // Desenha uma string
    // ssd1306_draw_string(&ssd, "Cor", 38, 16);  // Desenha uma string
    // ssd1306_draw_string(&ssd, "Cor", 68, 16);  // Desenha uma string
    // ssd1306_draw_string(&ssd, "Cor", 98, 16);  // Desenha uma string
    ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
    ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
    ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
    ssd1306_send_data(&ssd);                           // Atualiza o display
    sleep_ms(700);
  }
}