#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu6050.h" 

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema de detecção de queda...");

    esp_err_t err = mpu6050_init_sensor();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 inicializado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar o MPU6050. Erro: %s", esp_err_to_name(err));
        return; 
    }

    mpu6050_data_t sensor_data;

    while (1) {
        if (mpu6050_read_data(&sensor_data) == ESP_OK) {
            // Converte os dados brutos para unidades físicas
            float acc_x_g = sensor_data.acc_x / 16384.0;
            float acc_y_g = sensor_data.acc_y / 16384.0;
            float acc_z_g = sensor_data.acc_z / 16384.0;

            float gyro_x_deg = sensor_data.gyro_x / 131.0;
            float gyro_y_deg = sensor_data.gyro_y / 131.0;
            float gyro_z_deg = sensor_data.gyro_z / 131.0;

            ESP_LOGI(TAG, "--- Nova Leitura ---");
            ESP_LOGI(TAG, "Aceleração [g]   -> X: %.2f \tY: %.2f \tZ: %.2f", acc_x_g, acc_y_g, acc_z_g);
            ESP_LOGI(TAG, "Giroscópio [°/s] -> X: %.2f \tY: %.2f \tZ: %.2f", gyro_x_deg, gyro_y_deg, gyro_z_deg);
            ESP_LOGI(TAG, " ");
        } else {
            ESP_LOGE(TAG, "Falha na leitura dos dados do sensor.");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}