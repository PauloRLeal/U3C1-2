#include "pico/stdlib.h"
#include "ili9341.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define AHT10_ADDR 0x38
#define SDA_PIN 0
#define SCL_PIN 1

// Definindo os pinos do display para comunicação SPI
ILI9341_t display = {
    .sck = 18,
    .mosi = 19,
    .cs = 17,
    .rst = 16,
    .dc = 8,
    .bl = 9,
    .orientation = 0x60
};

char conv_temperatura[26];
char conv_umidade[26];
char conv_alerta_temp[26];
char conv_alerta_umid[26];


void update_warnings(float temp, float hum){
  if (hum > 70){
    sprintf(conv_alerta_umid, "Umidade > 70 %%");
  } else {
    sprintf(conv_alerta_umid, "Umidade OK");
  }
  if (temp < 20){
    sprintf(conv_alerta_temp, "Temperatura < 20.0 ºC");
  } else {
    sprintf(conv_alerta_temp, "Temperatura OK");
  }
}


void update_display(){
  ILI9341_clear(&display);

  ILI9341_drawText(&display, 0, 232, "------- Leituras --------", RGB_to_RGB565(255, 255, 255), 2, 2);
  ILI9341_drawText(&display, 0, 216, conv_temperatura, RGB_to_RGB565(255, 255, 255), 2, 2);
  ILI9341_drawText(&display, 0, 200, conv_umidade, RGB_to_RGB565(255, 255, 255), 2, 2);
  ILI9341_drawText(&display, 0, 168, "-------- Alertas! --------", RGB_to_RGB565(255, 255, 255), 2, 2);
  ILI9341_drawText(&display, 0, 152, conv_alerta_temp, RGB_to_RGB565(255, 255, 255), 2, 2);
  ILI9341_drawText(&display, 0, 136, conv_alerta_umid, RGB_to_RGB565(255, 255, 255), 2, 2);

}

void aht10_init() {
    uint8_t cmd[] = {0xBE, 0x08, 0x00};
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    sleep_ms(20);
}

void aht10_trigger_measurement() {
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    sleep_ms(80); // tempo de espera da medição
}

bool aht10_read(float *temperature, float *humidity) {
    uint8_t data[6];
    int res = i2c_read_blocking(I2C_PORT, AHT10_ADDR, data, 6, false);
    if (res != 6) return false;

    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] >> 4) & 0x0F);
    uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    *humidity = ((float)hum_raw / 1048576.0f) * 100.0f;
    *temperature = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;
    return true;
}

int main() {
    // Inicializa o sistema
    stdio_init_all();

    // Inicializa o display ILI9341
    ILI9341_init(&display);

    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    aht10_init();
    sleep_ms(100);

    while (1) {
        aht10_trigger_measurement();
        float temp, hum;
        if (aht10_read(&temp, &hum)) {
            printf("Temperatura: %.2f °C, Umidade: %.2f %%\n", temp, hum);
            sprintf(conv_temperatura, "Temperatura: %.2f °C", temp);
            sprintf(conv_umidade, "Umidade: %.2f %%", hum);
            update_warnings(temp, hum);
        } else {
            printf("Failed to read AHT10\n");
            sprintf(conv_temperatura, "Falha em ler a temperatura");
            sprintf(conv_umidade, "Falha em ler a umidade", hum);
        }
        update_display();
        sleep_ms(1000);
    }

    return 0;
}
