#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

// Definições dos registradores do MPU6050
#define MPUREG_PWR_MGMT_1     0x6B
#define MPUREG_ACCEL_CONFIG   0x1C
#define MPUREG_WHO_AM_I       0x75

// Tentar ambos os endereços possíveis do MPU6050
const int I2C_ADDRESSES[] = {0x68, 0x69}; // AD0 em LOW ou HIGH
const int I2C_SDA_GPIO = 20;
const int I2C_SCL_GPIO = 21;

int mpu_address = 0; // Será definido durante a inicialização

bool mpu_init() {
    uint8_t who_am_i;
    uint8_t reg = MPUREG_WHO_AM_I;
    
    // Tentar ambos os endereços possíveis
    for (int i = 0; i < 2; i++) {
        int addr = I2C_ADDRESSES[i];
        i2c_write_blocking(i2c_default, addr, &reg, 1, true);
        if (i2c_read_blocking(i2c_default, addr, &who_am_i, 1, false) > 0) {
            printf("Dispositivo encontrado no endereco 0x%02X, WHO_AM_I: 0x%02X\n", addr, who_am_i);
            if (who_am_i == 0x68 || who_am_i == 0x34) { // Aceitar ambos os valores
                mpu_address = addr;
                return true;
            }
        }
    }
    
    printf("Erro: MPU6050 nao encontrado em nenhum endereco!\n");
    return false;
}

void i2c_task(void *p) {
    // Inicialização do I2C
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_GPIO);
    gpio_pull_up(I2C_SCL_GPIO);

    if (!mpu_init()) {
        vTaskDelete(NULL);
        return;
    }

    // Reset do dispositivo para estado padrão
    uint8_t buf_write[2];
    buf_write[0] = MPUREG_PWR_MGMT_1;
    buf_write[1] = 1 << 7;  // Bit DEVICE_RESET
    i2c_write_blocking(i2c_default, mpu_address, buf_write, 2, false);
    
    // Pequeno delay para garantir o reset
    vTaskDelay(pdMS_TO_TICKS(100));

    // Configurar o acelerômetro para 4G (AFS_SEL = 01)
    buf_write[0] = MPUREG_ACCEL_CONFIG;
    buf_write[1] = 1 << 3;  // Bit AFS_SEL0 = 1 (AFS_SEL1 permanece 0)
    i2c_write_blocking(i2c_default, mpu_address, buf_write, 2, false);

    printf("MPU6050 configurado com sucesso:\n");
    printf("- Reset completo\n");
    printf("- Acelerometro configurado para 4G\n");

    while (1) {
        // Aqui você pode adicionar a leitura dos dados do sensor
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    stdio_init_all();
    printf("Iniciando RTOS...\n");

    xTaskCreate(i2c_task, "i2c_task", 4095, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true) {
        // Nada aqui - o scheduler FreeRTOS cuida das tarefas
    }
}