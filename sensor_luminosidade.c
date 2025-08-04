#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "st7789/st7789.h"

#define I2C_SENSOR_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define SENSOR_ADDRESS 0x23
#define COLOR_BLACK 0x0000
#define COLOR_GREEN 0x07E0
#define COLOR_YELLOW 0xFF00
#define SERVO_GPIO 2

// Dimensões do display
const int lcd_width = 240;
const int lcd_height = 320;

uint slice_num;
uint chan_num; 

// Configuração do display 
const struct st7789_config lcd_config = {
    .spi = spi0,
    .gpio_din = PICO_DEFAULT_SPI_TX_PIN,
    .gpio_clk = PICO_DEFAULT_SPI_SCK_PIN,
    .gpio_cs = -1,
    .gpio_dc  = 4,
    .gpio_rst = 20
};

void exibir_dados(uint16_t luminosidade);
bool read_bh1750(uint16_t *luminosidade);
void ajustar_servo(uint16_t lux);
void init_bh1750();
void setup();       

int main()
{
    setup();

    init_bh1750();

    while (true) {
        uint16_t luminosidade = 0;

        if(read_bh1750(&luminosidade)) {
            printf("Luminosidade: %d\n", luminosidade);
            exibir_dados(luminosidade);
        } else {
            printf("Erro ao tentar ler o BH1750\n");
        }

        ajustar_servo(luminosidade);

        sleep_ms(1000);
    }
}

// Configura os periféricos do microcontrolador
void setup() {
    stdio_init_all();

    st7789_init(&lcd_config, lcd_width, lcd_height);
    st7789_fill(COLOR_BLACK);

    sleep_ms(4000);

    i2c_init(I2C_SENSOR_PORT, 400 * 1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    gpio_init(SERVO_GPIO);
    gpio_set_dir(SERVO_GPIO, GPIO_OUT);

    gpio_set_function(SERVO_GPIO, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_GPIO);
    chan_num = pwm_gpio_to_channel(SERVO_GPIO);

    pwm_set_wrap(slice_num, 65535); 
    pwm_set_clkdiv(slice_num, 125); 

    pwm_set_wrap(slice_num, 25000 - 1); 
    pwm_set_clkdiv_int_frac(slice_num, 100, 0); 

    st7789_clear();
}

// Ajusta a posição do servo dependendo da luminosidade
void ajustar_servo(uint16_t lux) {
    int angle;

    if(lux > 720) {
        angle = 720;
    } else if(lux > 620) {
        angle = 620;
    } else if(lux > 560) {
        angle = 560;
    } else {
        angle = 500;
    }

    pwm_set_enabled(slice_num, true);

    for (int pos = 500; pos <= angle; pos += 10) { // Adjust range for your servo
            pwm_set_chan_level(slice_num, chan_num, pos);
            sleep_ms(20);
    }

    pwm_set_gpio_level(SERVO_GPIO, 0);
    pwm_set_enabled(slice_num, false);
}

// Inicializa o sensor BH1750
void init_bh1750() {
    uint8_t buffer[1] = {0x11};
    i2c_write_blocking(I2C_SENSOR_PORT, SENSOR_ADDRESS, &buffer[0], 1, false);
    sleep_ms(180);
}

// Ler dados dados do sensor BH1750
bool read_bh1750(uint16_t *luminosidade) {
    uint8_t buffer[2] = {0};

    int ret = i2c_read_blocking(I2C_SENSOR_PORT, SENSOR_ADDRESS, buffer, 2, false);

    if(ret != 2) {
        printf("Erro ao ler o sensor\n");
        return false;
    }

    *luminosidade = ((uint16_t)(buffer[0] << 8) | buffer[1]) / 1.2;

    return true;
}

// Imprime os dados no display LCD
void exibir_dados(uint16_t luminosidade) {
    char buffer_temp[8];

    snprintf(buffer_temp, sizeof(buffer_temp), 
        "%d LUX", luminosidade);
 
    st7789_clear();

    st7789_draw_text(12, 120, "LUMINOSIDADE", COLOR_GREEN, COLOR_BLACK, 3);
    st7789_draw_text(50, 175, buffer_temp, COLOR_YELLOW, COLOR_BLACK, 4);
} 