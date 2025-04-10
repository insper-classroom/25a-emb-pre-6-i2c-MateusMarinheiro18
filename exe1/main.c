#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#define MPUREG_PWR_MGMT_1     0x6B
#define MPUREG_ACCEL_CONFIG   0x1C
#define MPUREG_WHO_AM_I       0x75

const int I2C_CHIP_ADDRESS = 0x68;
const int I2C_SDA_GPIO = 20;
const int I2C_SCL_GPIO = 21;

void i2c_task(void *p) {
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_GPIO);
    gpio_pull_up(I2C_SCL_GPIO);

    uint8_t who_am_i;
    uint8_t reg = MPUREG_WHO_AM_I;
    i2c_write_blocking(i2c_default, I2C_CHIP_ADDRESS, &reg, 1, true);
    i2c_read_blocking(i2c_default, I2C_CHIP_ADDRESS, &who_am_i, 1, false);
    
    printf("WHO_AM_I: 0x%02X\n", who_am_i);
    if (who_am_i != 0x68) {
        printf("Erro: Dispositivo MPU6050 nao encontrado!\n");
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf_write[2];
    buf_write[0] = MPUREG_PWR_MGMT_1;
    buf_write[1] = 1 << 7;
    i2c_write_blocking(i2c_default, I2C_CHIP_ADDRESS, buf_write, 2, false);
    
    vTaskDelay(pdMS_TO_TICKS(100));

    buf_write[0] = MPUREG_ACCEL_CONFIG;
    buf_write[1] = 1 << 3;  
    i2c_write_blocking(i2c_default, I2C_CHIP_ADDRESS, buf_write, 2, false);

    printf("MPU6050 configurado com sucesso:\n");
    printf("- Reset completo\n");
    printf("- Acelerometro configurado para 4G\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    stdio_init_all();
    printf("Iniciando RTOS...\n");

    xTaskCreate(i2c_task, "i2c_task", 4095, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true) {
    }
}