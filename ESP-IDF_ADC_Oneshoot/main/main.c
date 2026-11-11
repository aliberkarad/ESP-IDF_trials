#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define ADC_CHANNEL     ADC_CHANNEL_6 // ESP32 GPIO34
static const char *TAG = "ADC_ONESHOT";

void app_main(void)
{
    // 1. ADC Birimi (Unit) Başlatma
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    // 2. ADC Kanal (Channel) ve Zayıflatma (Attenuation) Yapılandırması
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12-Bit çözünürlük (0 - 4095)
        .atten = ADC_ATTEN_DB_12,         // 0 - 3.3V voltaj aralığı okuma
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config);

    int raw_value = 0;

    while (1) {
        // 3. Ham (Raw) Analog Değeri Oku
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_value);

        ESP_LOGI(TAG, "ADC Ham (Raw) Değer: %d / 4095", raw_value);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
