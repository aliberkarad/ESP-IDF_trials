#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_CHANNEL     ADC_CHANNEL_6 // ESP32 GPIO34 / ESP32-S3 GPIO7
static const char *TAG = "ADC_CALI_LESSON";

void app_main(void)
{
    // 1. ADC Oneshot Donanım Birimi Yapılandırması
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    // 2. Kanal Ayarları (12-Bit Çözünürlük, 12 dB Zayıflatma -> 0 - 3.3V)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config);

    // 3. eFuse Kalibrasyon Sürücüsünü Başlatma
    adc_cali_handle_t cali_handle = NULL;
    bool do_calibration = false;

    #if CONFIG_ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    // Çip fabrikasyon eFuse referans verisini oku ve kalibrasyon şemasını aç
    if (adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle) == ESP_OK) {
        do_calibration = true;
        ESP_LOGI(TAG, "eFuse Kalibrasyon Şeması Başarıyla Yüklendi!");
    }
    #endif

    int raw_value = 0;
    int voltage_mv = 0;

    while (1) {
        // Ham (Raw) ADC Okuma (0 - 4095 arası tamsayı)
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_value);

        if (do_calibration) {
            // 4. S-Eğrisi Sapmasını Düzelterek Gerçek mV Değerine Çevir
            adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv);

            ESP_LOGI(TAG, "Ham: %d / 4095  |  Gerilim: %d mV  (%.2f V)", 
                     raw_value, voltage_mv, voltage_mv / 1000.0);
        } else {
            ESP_LOGW(TAG, "Kalibrasyon Yapılamadı! Ham Değer: %d", raw_value);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}