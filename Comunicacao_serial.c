#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/time.h"
#include "ws2812.pio.h" // Biblioteca para controle dos LEDs WS2812

// Definições de pinos
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDRESS 0x3C
#define WS2812_PIN 7
#define LED_RGB_RED 13
#define LED_RGB_GREEN 11
#define LED_RGB_BLUE 12
#define BUTTON_A 5
#define BUTTON_B 6
#define NUM_LEDS 25  // Matriz 5x5 de LEDs WS2812

// Definições do display
#define WIDTH 128
#define HEIGHT 64

// Variáveis de estado
bool greenLedState = false;
bool blueLedState = false;

// Estrutura do display
ssd1306_t ssd;

// Padrões para os números de 0 a 9 (matriz 5x5)
const uint32_t patterns[10][25] = {
    // 0
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    // 1
    {0, 1, 1, 1, 0,
     0, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     0, 1, 1, 0, 0,
     0, 0, 1, 0, 0},

    // 2
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    // 3
    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    // 4
    {1, 0, 0, 0, 0,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1},

    // 5
    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1},

    // 6
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1},

    // 7
    {0, 0, 0, 1, 0,
     0, 0, 1, 0, 0,
     0, 1, 0, 0, 0,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    // 8
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    // 9
    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}
};

// Função para exibir números na matriz de LEDs WS2812
void display_number(int number) {
    // Verifica se o número está no intervalo válido (0-9)
    if (number < 0 || number > 9) return;

    // Define as cores dos LEDs com base no padrão
    for (int i = 0; i < NUM_LEDS; i++) {
        uint32_t color = (patterns[number][i] == 1) ? 0x0000FF : 0x000000;  // Azul ou apagado
        pio_sm_put_blocking(pio0, 0, color << 8u);  // Envia o valor para o WS2812
    }
}

// Inicialização do I2C
void initI2C() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// Inicialização do display
void initDisplay() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Inicialização dos LEDs WS2812
void initWS2812() {
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
}

// Função para tratar interrupções dos botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    static absolute_time_t ultimo_tempo_a = 0;
    static absolute_time_t ultimo_tempo_b = 0;

    if (gpio == BUTTON_A) {
        static absolute_time_t lastInterruptTime = 0;
        absolute_time_t now = get_absolute_time();

        // Debounce
        if (absolute_time_diff_us(lastInterruptTime, now) > 200000) {
            greenLedState = !greenLedState;
            gpio_put(LED_RGB_GREEN, greenLedState);

            // Atualização do display
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, greenLedState ? "Led Verde Ligado" : "Led Verde Desligado", 10, 10);
            ssd1306_send_data(&ssd);

            // Envio de mensagem ao Serial Monitor
            printf(greenLedState ? "Led Verde Ligado\n" : "Led Verde Desligado\n");
        }
        lastInterruptTime = now;
    } else if (gpio == BUTTON_B) {
        static absolute_time_t lastInterruptTime = 0;
        absolute_time_t now = get_absolute_time();

        // Debounce
        if (absolute_time_diff_us(lastInterruptTime, now) > 200000) {
            blueLedState = !blueLedState;
            gpio_put(LED_RGB_BLUE, blueLedState);

            // Atualização do display
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, blueLedState ? "Led Azul Ligado" : "Led Azul Desligado", 10, 30);
            ssd1306_send_data(&ssd);

            // Envio de mensagem ao Serial Monitor
            printf(blueLedState ? "Led Azul Ligado\n" : "Led Azul Desligado\n");
        }
        lastInterruptTime = now;
    }
}

// Inicialização dos botões com interrupções
void initButtons() {
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

// Função para tratar caracteres recebidos via UART
// void handleUART() {
//     if (uart_is_readable(uart0)) {
//         char receivedChar = uart_getc(uart0);

//         // Exibição do caractere no display
//         ssd1306_fill(&ssd, false);
//         ssd1306_draw_string(&ssd, "Caractere: ", 10, 10);
//         char buffer[2] = {receivedChar, '\0'};
//         ssd1306_draw_string(&ssd, buffer, 80, 10);
//         ssd1306_send_data(&ssd);

//         // Exibição de número na matriz WS2812 (se for um número)
//         if (receivedChar >= '0' && receivedChar <= '9') {
//             int number = receivedChar - '0';
//             display_number(number); // Exibe o número na matriz de LEDs
//         }
//     }
// }

void handleUSB() {
    // Verifica se há caracteres disponíveis na entrada USB
    int receivedChar = getchar();

    if (receivedChar != PICO_ERROR_TIMEOUT) { // Se um caractere foi recebido
        // Exibição do caractere no display
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Caractere: ", 10, 10);
        char buffer[2] = {(char)receivedChar, '\0'};
        ssd1306_draw_string(&ssd, buffer , 90, 10);
        ssd1306_send_data(&ssd);
        printf("Caractere: %c", receivedChar);

        // Exibição de número na matriz WS2812 (se for um número)
        if (receivedChar >= '0' && receivedChar <= '9') {
            int number = receivedChar - '0';
            display_number(number); // Exibe o número na matriz de LEDs
        }
    }
}

// Função principal
int main() {
    stdio_init_all(); // Inicializa a comunicação serial
    initI2C(); // Inicializa o I2C
    initDisplay(); // Inicializa o display
    initWS2812(); // Inicializa os LEDs WS2812
    initButtons(); // Inicializa os botões

    // Configuração dos pinos dos LEDs RGB
    gpio_init(LED_RGB_RED);
    gpio_init(LED_RGB_GREEN);
    gpio_init(LED_RGB_BLUE);
    gpio_set_dir(LED_RGB_RED, GPIO_OUT);
    gpio_set_dir(LED_RGB_GREEN, GPIO_OUT);
    gpio_set_dir(LED_RGB_BLUE, GPIO_OUT);
    
    while (true) {
        if (stdio_usb_connected()) {
            bool cor = true;
            cor = !cor;
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo

            handleUSB(); // Verifica se há caracteres recebidos via UART
        }
        sleep_ms(100); // Pequeno delay para evitar sobrecarga
    }
}