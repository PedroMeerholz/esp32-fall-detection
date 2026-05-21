#pragma once

#include <stdint.h>
#include "esp_err.h"

// Estrutura para armazenar as leituras do MPU6050
typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} mpu6050_data_t;

// Inicializa o barramento I2C e o sensor MPU6050
esp_err_t mpu6050_init_sensor(void);

// Lê os dados atuais do sensor e popula a estrutura fornecida
esp_err_t mpu6050_read_data(mpu6050_data_t *data);