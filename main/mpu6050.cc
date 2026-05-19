#include "mpu6050.h"
#include "driver/i2c.h"

// Configurações do I2C
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

static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {}; 
    
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t mpu6050_init_sensor(void) {
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) return err;

    uint8_t init_data[2] = {MPU6050_PWR_MGMT_1_REG, 0x00};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, init_data, 2, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t mpu6050_read_data(mpu6050_data_t *out_data) {
    if (out_data == nullptr) return ESP_ERR_INVALID_ARG;

    uint8_t raw_data[14];
    uint8_t reg_addr = MPU6050_ACCEL_XOUT_H_REG;

    // Leitura dos 14 bytes de dados (Aceleração + Temperatura + Giroscópio)
    esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg_addr, 1, raw_data, 14, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    if (err == ESP_OK) {
        // Preenche Aceleração (Bytes 0 a 5)
        out_data->acc_x  = (raw_data[0] << 8) | raw_data[1];
        out_data->acc_y  = (raw_data[2] << 8) | raw_data[3];
        out_data->acc_z  = (raw_data[4] << 8) | raw_data[5];
        
        // Preenche Giroscópio (Bytes 8 a 13)
        out_data->gyro_x = (raw_data[8] << 8) | raw_data[9];
        out_data->gyro_y = (raw_data[10] << 8) | raw_data[11];
        out_data->gyro_z = (raw_data[12] << 8) | raw_data[13];
    }

    return err;
}