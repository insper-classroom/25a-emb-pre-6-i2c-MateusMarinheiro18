#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

// Endereços I2C dos dispositivos
#define BMP280_ADDRESS      0x76  // Endereço padrão do BMP280 (SDO conectado a GND)
#define MPU6050_ADDRESS     0x68  // Endereço padrão do MPU6050 (AD0 conectado a GND)

// Registradores dos dispositivos
#define BMP280_REG_ID       0xD0  // Registrador de ID do BMP280
#define MPU6050_REG_WHO_AM_I 0x75 // Registrador WHO_AM_I do MPU6050

// Configuração dos pinos I2C
const int I2C_SDA_GPIO = 20;
const int I2C_SCL_GPIO = 21;

void i2c_task(void *p) {
    // Inicialização do I2C
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_GPIO);
    gpio_pull_up(I2C_SCL_GPIO);

    uint8_t buffer[1];
    uint8_t reg_address;

    printf("Iniciando leitura dos dispositivos I2C...\n");

    // Leitura do ID do BMP280
    reg_address = BMP280_REG_ID;
    i2c_write_blocking(i2c_default, BMP280_ADDRESS, &reg_address, 1, true);
    i2c_read_blocking(i2c_default, BMP280_ADDRESS, buffer, 1, false);
    printf("BMP280 ID: 0x%X\n", buffer[0]);

    // Leitura do WHO_AM_I do MPU6050
    reg_address = MPU6050_REG_WHO_AM_I;
    i2c_write_blocking(i2c_default, MPU6050_ADDRESS, &reg_address, 1, true);
    i2c_read_blocking(i2c_default, MPU6050_ADDRESS, buffer, 1, false);
    printf("MPU6050 WHO_AM_I: 0x%X\n", buffer[0]);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    stdio_init_all();
    printf("Sistema Iniciado\n");

    xTaskCreate(i2c_task, "i2c_task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true) {
    }
}