#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu6050.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char *TAG = "MAIN";

extern const unsigned char g_model[];
extern const unsigned int g_model_len;

// Área de memória para os cálculos da IA
constexpr int kTensorArenaSize = 8 * 1024; // 8 KB
uint8_t tensor_arena[kTensorArenaSize];

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema de detecção de queda com Edge AI...");

    // 1. Inicializa o Sensor
    if (mpu6050_init_sensor() == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 inicializado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar o MPU6050.");
        return; 
    }

    // 2. Carrega o Modelo do TensorFlow Lite
    const tflite::Model* model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Versão do modelo TFLite não suportada!");
        return;
    }

    // 3. Configura os Operadores Matemáticos (OpResolver)
    // Dica: Adicione aqui as operações que sua rede neural usa (ex: Conv2D, Softmax, etc)
    tflite::MicroMutableOpResolver<5> resolver;
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddSoftmax();
    resolver.AddReshape();

    // 4. Instancia o Interpretador
    tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, kTensorArenaSize);

    // 5. Aloca memória para os tensores de entrada/saída
    if (interpreter.AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "Falha ao alocar memória para os tensores!");
        return;
    }

    // Ponteiros para facilitar o acesso à entrada e saída da IA
    TfLiteTensor* input = interpreter.input(0);
    TfLiteTensor* output = interpreter.output(0);

    mpu6050_data_t sensor_data;

    while (1) {
        if (mpu6050_read_data(&sensor_data) == ESP_OK) {
            // Converte para unidades físicas
            float acc_x = sensor_data.acc_x / 16384.0f;
            float acc_y = sensor_data.acc_y / 16384.0f;
            float acc_z = sensor_data.acc_z / 16384.0f;
            float gyro_x = sensor_data.gyro_x / 131.0f;
            float gyro_y = sensor_data.gyro_y / 131.0f;
            float gyro_z = sensor_data.gyro_z / 131.0f;

            // Preparação dos dados para a inferência
            input->data.f[0] = acc_x;
            input->data.f[1] = acc_y;
            input->data.f[2] = acc_z;
            input->data.f[3] = gyro_x;
            input->data.f[4] = gyro_y;
            input->data.f[5] = gyro_z;

            // Realiza a inferência
            if (interpreter.Invoke() != kTfLiteOk) {
                ESP_LOGE(TAG, "Falha ao executar a inferência!");
            } else {
                // Resultado da inferência
                float prob_queda = output->data.f[0]; 
                
                ESP_LOGI(TAG, "Confiança de Queda: %.2f%%", prob_queda * 100.0f);

                if (prob_queda > 0.80f) { // Aplica limiar de 80%
                    ESP_LOGW(TAG, ">>> ALERTA: QUEDA DETECTADA! <<<");
                } else {
                    ESP_LOGI(TAG, "Nenhuma queda detectada.");
                }
            }
        } else {
            ESP_LOGE(TAG, "Falha na leitura dos dados do sensor.");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}