#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#define MPUREG_PWR_MGMT_1     0x6B
#define MPUREG_ACCEL_CONFIG   0x1C
#define MPUREG_WHO_AM_I       0x75

typedef struct {
    int address;
    i2c_inst_t *i2c;
} mpu_context_t;

bool mpu_init(mpu_context_t *ctx) {
    uint8_t who_am_i;
    uint8_t reg = MPUREG_WHO_AM_I;
    const int possible_addresses[] = {0x68, 0x69};
    
    for (int i = 0; i < 2; i++) {
        int addr = possible_addresses[i];
        i2c_write_blocking(ctx->i2c, addr, &reg, 1, true);
        if (i2c_read_blocking(ctx->i2c, addr, &who_am_i, 1, false) > 0) {
            printf("Dispositivo encontrado no endereco 0x%02X, WHO_AM_I: 0x%02X\n", addr, who_am_i);
            if (who_am_i == 0x68 || who_am_i == 0x34) {
                ctx->address = addr;
                return true;
            }
        }
    }
    
    printf("Erro: MPU6050 nao encontrado!\n");
    return false;
}

void mpu_configure(mpu_context_t *ctx) {
    // Reset do dispositivo
    uint8_t buf_write[2] = {MPUREG_PWR_MGMT_1, 1 << 7};
    i2c_write_blocking(ctx->i2c, ctx->address, buf_write, 2, false);
    vTaskDelay(pdMS_TO_TICKS(100));

    buf_write[0] = MPUREG_ACCEL_CONFIG;
    buf_write[1] = 1 << 3;
    i2c_write_blocking(ctx->i2c, ctx->address, buf_write, 2, false);
}

void i2c_task(void *p) {
    const int sda_pin = 20;
    const int scl_pin = 21;
    
    i2c_inst_t *i2c = i2c0;
    i2c_init(i2c, 400 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    mpu_context_t mpu_ctx = {
        .i2c = i2c,
        .address = 0
    };

    if (!mpu_init(&mpu_ctx)) {
        vTaskDelete(NULL);
        return;
    }

    mpu_configure(&mpu_ctx);
    printf("MPU6050 configurado com sucesso!\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    stdio_init_all();
    printf("Iniciando sistema...\n");

    xTaskCreate(i2c_task, "i2c_task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    while (1) {}
}