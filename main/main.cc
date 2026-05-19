#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2c.h"
#include "esp_log.h"

// Definições dos pinos I2C para o ESP32-S2 (Altere se necessário no seu diagrama)
#define I2C_MASTER_SDA_IO           GPIO_NUM_8      
#define I2C_MASTER_SCL_IO           GPIO_NUM_9     
#define I2C_MASTER_NUM              I2C_NUM_0 
#define I2C_MASTER_FREQ_HZ          400000 
#define I2C_MASTER_TX_BUF_DISABLE   0      
#define I2C_MASTER_RX_BUF_DISABLE   0      
#define I2C_TIMEOUT_MS              1000

// Registradores do MPU6050
#define MPU6050_ADDR                0x68
#define MPU6050_PWR_MGMT_1_REG      0x6B
#define MPU6050_ACCEL_XOUT_H_REG    0x3B

static const char *TAG = "MPU6050";

// Função para inicializar a porta I2C do ESP32-S2
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {}; // Cria a estrutura vazia e preenche com zeros (Importante!)
    
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ; // Agora o C++ aceita isso sem problemas
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Função para acordar o MPU6050 (Ele liga em modo Sleep por padrão)
static esp_err_t mpu6050_init(void) {
    // Escreve 0 no registrador de energia (0x6B) para acordar o sensor
    uint8_t data[2] = {MPU6050_PWR_MGMT_1_REG, 0x00};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, data, 2, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
}

// Ponto de entrada do ESP-IDF
extern "C" void app_main(void) {
    // 1. Inicializa o barramento I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C inicializado com sucesso.");

    // 2. Inicializa o MPU6050
    esp_err_t err = mpu6050_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 acordado e pronto!");
    } else {
        ESP_LOGE(TAG, "Falha ao encontrar o MPU6050. Erro: %s", esp_err_to_name(err));
        return; // Encerra o programa se não encontrar o sensor
    }

    uint8_t data[14]; // Buffer para receber os dados puros (14 bytes)

    while (1) {
        // Registrador inicial de leitura (Aceleração X High)
        uint8_t reg_addr = MPU6050_ACCEL_XOUT_H_REG;

        // Lê 14 bytes em sequência a partir do registrador 0x3B
        // Ordem: Accel(6), Temp(2), Gyro(6)
        err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg_addr, 1, data, 14, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
        
        if (err == ESP_OK) {
            // Recombina os bytes altos (High) e baixos (Low) - O MPU6050 retorna dados de 16 bits
            int16_t accel_x = (data[0] << 8) | data[1];
            int16_t accel_y = (data[2] << 8) | data[3];
            int16_t accel_z = (data[4] << 8) | data[5];
            
            int16_t temp_raw = (data[6] << 8) | data[7];
            
            int16_t gyro_x  = (data[8] << 8) | data[9];
            int16_t gyro_y  = (data[10] << 8) | data[11];
            int16_t gyro_z  = (data[12] << 8) | data[13];

            // Converte a temperatura bruta para Graus Celsius (fórmula do datasheet)
            float temperature = (temp_raw / 340.0) + 36.53;

            // Imprime no console (Monitor Serial do ESP-IDF)
            ESP_LOGI(TAG, "--- Leitura ---");
            ESP_LOGI(TAG, "Aceleração [RAW] -> X: %d | Y: %d | Z: %d", accel_x, accel_y, accel_z);
            ESP_LOGI(TAG, "Giroscópio [RAW] -> X: %d | Y: %d | Z: %d", gyro_x, gyro_y, gyro_z);
            ESP_LOGI(TAG, "Temperatura      -> %.2f °C", temperature);
            ESP_LOGI(TAG, " ");
        } else {
            ESP_LOGE(TAG, "Falha na leitura dos dados do sensor.");
        }

        // Equivalente ao delay() no FreeRTOS (Delay de 500ms)
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}