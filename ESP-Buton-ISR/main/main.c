#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO    GPIO_NUM_2

static const char *TAG = "ISR_LESSON";

// ISR ve Main Task arasında paylaşılan değişkenler volatile olmalıdır
static volatile uint32_t btn_press_count = 0;
static volatile int64_t last_isr_time = 0; // Debounce (ark) kontrolü için
static volatile bool led_state = false; // LED durumu

// ISR Fonksiyonu: FLASH yerine RAM'de (IRAM) çalışması şarttır!
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    int64_t current_time = esp_timer_get_time(); // Mikrosaniye cinsinden zaman
    
    // 200 ms (200.000 us) Debounce Kontrolü
    if (current_time - last_isr_time > 200000) {
        btn_press_count++;
        last_isr_time = current_time;
    }
}

void app_main(void)
{
    // 1. LED Çıkış Pini Ayarı
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);

    // 2. Buton Giriş ve Kesme (Interrupt) Ayarı
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,     // Buton boştayken HIGH (3.3V)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE        // Butona basıldığında (HIGH -> LOW) tetikle
    };
    gpio_config(&btn_conf);

    // 3. Global ISR Servisini Kur
    gpio_install_isr_service(0);

    // 4. Pin ile ISR Fonksiyonunu Birbirine Bağla
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);

    ESP_LOGI(TAG, "Kesme sistemi hazır. Butona basılması bekleniyor...");

    uint32_t last_handled_count = 0;

    while (1) {
        // Değişiklik var mı kontrol et
        if (btn_press_count != last_handled_count) {
            last_handled_count = btn_press_count;
            
            ESP_LOGI(TAG, "Butona Basıldı! Toplam Sayı: %lu", last_handled_count);
            
            // LED'in durumunu değiştir (Toggle)
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state);
            //gpio_set_level(LED_GPIO, last_handled_count % 2); // Sayım taşması hata verebilir, bu yüzden led_state kullanıyoruz
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // CPU'yu yorma
    }
}