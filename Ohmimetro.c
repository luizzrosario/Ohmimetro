// Description: Ohmimetro com display OLED e LEDs WS2812 - Luiz Rosário
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "ws2812.pio.h"

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}
// Aqui termina o trecho para modo BOOTSEL com botão B

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A
#define WS2812_PIN 7
#define MAX_LEDS 25
#define TAMANHO_E24 (sizeof(resistores_E24) / sizeof(resistores_E24[0]))

// Definições para o Ohmimetro
int R_conhecido = 10000;   // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

// Definições para a matriz de LEDs
static int sm = 0;
static PIO pio = pio0;
static uint32_t grb[MAX_LEDS];

// Variável para o display OLED (antes de declarar a função para não dar erro de compilação)
ssd1306_t ssd;

// Definições para as faixa dos resistores E24
const float resistores_E24[] = {
    510, 560, 620, 680, 750, 820, 910,
    1000, 1100, 1200, 1300, 1500, 1600, 1800, 2000,
    2200, 2400, 2700, 3000, 3300, 3600, 3900, 4300,
    4700, 5100, 5600, 6200, 6800, 7500, 8200, 9100,
    10000, 11000, 12000, 13000, 15000, 16000, 18000,
    20000, 22000, 24000, 27000, 30000, 33000, 36000,
    39000, 43000, 47000, 51000, 56000, 62000, 68000,
    75000, 82000, 91000, 100000};

// Definições para as cores das faixas dos resistores
const char *cores[] = {
    "PRE", // 0 - Preto
    "MAR", // 1 - Marrom
    "VER", // 2 - Vermelho
    "LAR", // 3 - Laranja
    "AMA", // 4 - Amarelo
    "VRD", // 5 - Verde
    "AZU", // 6 - Azul
    "VIO", // 7 - Violeta
    "CIN", // 8 - Cinza
    "BRA", // 9 - Branco
    "DOU", // 10 - Dourado
    "PRA"  // 11 - Prata
};

// Função para configurar a cor de um pixel específico
static void configurar_pixel(uint32_t porcentagem_cores[][3], int index, const uint32_t cor[3])
{
  porcentagem_cores[index][0] = cor[0]; // Vermelho
  porcentagem_cores[index][1] = cor[1]; // Verde
  porcentagem_cores[index][2] = cor[2]; // Azul
}

// Função para montar o buffer de cores com base no símbolo escolhido
static void montar_buffer_simbolo(int numero, uint32_t porcentagem_cores[][3], int index)
{
  // Definições das cores para cada faixa de resistor
  const uint32_t cores[12][3] = {
      {0, 0, 0},       // PRE - Preto
      {50, 25, 0},     // MAR - Marrom
      {100, 0, 0},     // VER - Vermelho
      {100, 50, 0},    // LAR - Laranja
      {100, 100, 0},   // AMA - Amarelo
      {0, 100, 0},     // VRD - Verde
      {0, 0, 100},     // AZU - Azul
      {50, 0, 50},     // VIO - Violeta
      {50, 50, 50},    // CIN - Cinza
      {100, 100, 100}, // BRA - Branco
      {85, 70, 0},     // DOU - Dourado
      {75, 75, 75}     // PRA - Prata
  };

  // Acende apenas o LED correspondente à faixa
  configurar_pixel(porcentagem_cores, index, cores[numero]);
}

// Função para enviar os dados para os LEDs WS2812
static inline void put_pixel(uint32_t pixel_grb)
{
  pio_sm_put_blocking(pio, sm, pixel_grb << 8u); // Já shiftado
}

// Função para enviar os dados para os LEDs WS2812
static void enviar_para_leds()
{
  for (int i = 0; i < MAX_LEDS; ++i)
  {
    put_pixel(grb[i]);
  }
}

// Função para converter porcentagens de cores RGB para o formato GRB
static void rgb_to_grb(uint32_t porcentagem_cores[][3])
{
  for (int i = 0; i < MAX_LEDS; i++)
  {
    uint8_t r = porcentagem_cores[i][0] ? 255 * (porcentagem_cores[i][0] / 100.0) : 0;
    uint8_t g = porcentagem_cores[i][1] ? 255 * (porcentagem_cores[i][1] / 100.0) : 0;
    uint8_t b = porcentagem_cores[i][2] ? 255 * (porcentagem_cores[i][2] / 100.0) : 0;
    grb[i] = (g << 16) | (r << 8) | b;
  }
}

// Função para encontrar o valor de resistor mais próximo na tabela E24
float encontrarValorMaisProximo(float resistor)
{
  float maisProximo = resistores_E24[0];
  float menorDiferenca = fabsf(resistor - resistores_E24[0]);

  for (int i = 1; i < TAMANHO_E24; i++)
  {
    float diferencaAtual = fabsf(resistor - resistores_E24[i]);
    if (diferencaAtual < menorDiferenca)
    {
      menorDiferenca = diferencaAtual;
      maisProximo = resistores_E24[i];
    }
  }

  return maisProximo;
}

// Função para mostrar as cores do resistor no display OLED
void mostrarCoresResistor(float valor)
{
  float valorProximo = encontrarValorMaisProximo(valor);
  int digito1, digito2, multiplicador = 0;
  int valorInt = (int)valorProximo;

  uint32_t porcentagem_cores[MAX_LEDS][3] = {0}; // Zerar tudo

  // Ajuste para pegar o valor dos 2 primeiros dígitos e multiplicador
  while (valorInt >= 100)
  {
    valorInt /= 10;
    multiplicador++;
  }

  digito1 = valorInt / 10;
  digito2 = valorInt % 10;

  // Exibir no display
  ssd1306_draw_string(&ssd, cores[digito1], 23, 20);       // Primeira faixa
  ssd1306_draw_string(&ssd, cores[digito2], 53, 20);       // Segunda faixa
  ssd1306_draw_string(&ssd, cores[multiplicador], 83, 20); // Multiplicador

  // Preencher LEDs: apenas 3 LEDs acesos, um para cada faixa
  montar_buffer_simbolo(digito1, porcentagem_cores, 14);       // Primeiro LED para o primeiro dígito
  montar_buffer_simbolo(digito2, porcentagem_cores, 12);       // Segundo LED para o segundo dígito
  montar_buffer_simbolo(multiplicador, porcentagem_cores, 10); // Terceiro LED para o multiplicador

  // Converter e enviar para LEDs
  rgb_to_grb(porcentagem_cores);
  enviar_para_leds();
}

int main()
{
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  // Configuração do display OLED
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  // Configuração do ADC
  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  // Configurando o ws2812
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

  for (int i = 0; i < MAX_LEDS; i++)
  {
    pio_sm_put_blocking(pio, sm, 0);
  }

  // Medição da tensão no pino ADC
  float tensao;
  char str_x[5];   // Buffer para armazenar a string
  char str_y[5];   // Buffer para armazenar a string
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

    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);

    // Desenha os textos no display
    ssd1306_draw_string(&ssd, str_Cor, 8, 6);
    mostrarCoresResistor(encontrarValorMaisProximo(R_x)); // Chama a função para mostrar as cores;
    ssd1306_draw_string(&ssd, "ADC", 13, 41);
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);
    ssd1306_draw_string(&ssd, str_x, 8, 52);
    ssd1306_draw_string(&ssd, str_y, 59, 52);

    // Atualiza o display
    ssd1306_send_data(&ssd);
    sleep_ms(700);
  }
}