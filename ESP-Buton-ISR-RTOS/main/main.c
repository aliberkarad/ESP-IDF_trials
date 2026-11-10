#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO    GPIO_NUM_2

static const char *TAG = "QUEUE_LESSON";

// 1. Kuyruğun (Posta Kutusu) RAM adresini tutan global değişken
static QueueHandle_t gpio_evt_queue = NULL;

// 2. KESME FONKSİYONU (ISR) - IRAM bellek üzerinde çalışır
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // Parametre olarak gelen (void*) 4 verisini uint32_t tipine çevirir
    uint32_t gpio_num = (uint32_t) arg; 

    // Yüksek öncelikli görev uyandı mı bayrağı (varsayılan pdFALSE = 0)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Pin numarasını (4) kesme-güvenli fonksiyon ile kuyruğa kopyalar
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);

    // Kuyruğa veri girince uykudaki yüksek öncelikli Task uyandıysa derhal o Task'a zıplar
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// 3. ANA GÖREV (TASK) - Kuyruktan veri gelene kadar uykuda bekler
static void gpio_task_example(void* arg)
{
    uint32_t io_num; // Kuyruktan çekilecek pin numarasının yazılacağı yerel değişken

    while (1) {
        // Kuyrukta veri var mı kontrol eder; yoksa portMAX_DELAY ile uykuda bekler
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Kuyruktan veri çekildi! Basılan Pin: GPIO %lu", io_num);
            
            // LED'in mevcut durumunu okuyup tersine çevirir (Toggle)
            gpio_set_level(LED_GPIO, !gpio_get_level(LED_GPIO));
        }
    }
}

void app_main(void)
{
    // Buton Pini (GPIO 4) Giriş ve Kesme Konfigürasyonu
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // Düşen kenar (HIGH -> LOW) tetiklemesi
    };
    gpio_config(&btn_conf);

    // LED Pini (GPIO 2) Çıkış Konfigürasyonu
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);

    // 10 eleman kapasiteli, her elemanı 4 bayt (uint32_t) olan Kuyruk (Queue) oluşturulur
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Kuyruğu dinleyecek FreeRTOS Görevini (Task) başlat (Öncelik: 10, Stack: 2048 Bayt)
    xTaskCreate(gpio_task_example, "gpio_task", 2048, NULL, 10, NULL);

    // Donanımsal Kesme Servisini kur ve Buton pinini ISR fonksiyonuna bağla
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);
}