#include <stdio.h>
#include <math.h>
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
constexpr int kTensorArenaSize = 64 * 1024; 
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
    tflite::MicroMutableOpResolver<6> resolver;
    // Para todas as camadas Dense (a base matemática da rede)
    resolver.AddFullyConnected(); 
    // Para todas as ativações activation='relu'
    resolver.AddRelu(); 
    // Para a ativação final activation='sigmoid'
    resolver.AddLogistic(); 
    // Essencial pois o modelo foi quantizado para INT8
    resolver.AddQuantize();   
    resolver.AddDequantize(); 
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

    float input_scale = input->params.scale;
    int input_zero = input->params.zero_point;
    
    float output_scale = output->params.scale;
    int output_zero = output->params.zero_point;

    mpu6050_data_t sensor_data;

    while (1) {
        if (mpu6050_read_data(&sensor_data) == ESP_OK) {
            // 1. Lê a unidade física (float)
            float acc_x = sensor_data.acc_x / 16384.0f;
            float acc_y = sensor_data.acc_y / 16384.0f;
            float acc_z = sensor_data.acc_z / 16384.0f;
            float gyro_x = sensor_data.gyro_x / 131.0f;
            float gyro_y = sensor_data.gyro_y / 131.0f;
            float gyro_z = sensor_data.gyro_z / 131.0f;

            // 2. Quantiza a entrada (Converte de Float para INT8)
            // Fórmula: int8 = (float / scale) + zero_point
            input->data.int8[0] = (int8_t)round((acc_x / input_scale) + input_zero);
            input->data.int8[1] = (int8_t)round((acc_y / input_scale) + input_zero);
            input->data.int8[2] = (int8_t)round((acc_z / input_scale) + input_zero);
            input->data.int8[3] = (int8_t)round((gyro_x / input_scale) + input_zero);
            input->data.int8[4] = (int8_t)round((gyro_y / input_scale) + input_zero);
            input->data.int8[5] = (int8_t)round((gyro_z / input_scale) + input_zero);

            // 3. Executa a inferência
            if (interpreter.Invoke() != kTfLiteOk) {
                ESP_LOGE(TAG, "Falha ao executar a inferência!");
            } else {
                // 4. Desquantiza a saída (Converte de INT8 de volta para Float/Porcentagem)
                // Pega a saída bruta do modelo (0 a 255 ou -128 a 127)
                int8_t saida_bruta = output->data.int8[0];
                
                // Fórmula: float = (int8 - zero_point) * scale
                float prob_queda = (saida_bruta - output_zero) * output_scale;
                
                ESP_LOGI(TAG, "Confiança de Queda: %.2f%%", prob_queda * 100.0f);

                if (prob_queda > 0.50f) {
                    ESP_LOGW(TAG, ">>> ALERTA: QUEDA DETECTADA! <<<");
                }
            }
        } else {
            ESP_LOGE(TAG, "Falha na leitura dos dados do MPU6050.");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}