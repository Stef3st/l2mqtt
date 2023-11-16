/* L2MQTT: Control LED colors with use of the MQTT protocol

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

static const char *TAG = "L2MQTT";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO pins for the 
   LED colors, or you can edit the following line and set a number here.
*/
#define RED_GPIO CONFIG_RED_GPIO
#define GREEN_GPIO CONFIG_GREEN_GPIO
#define BLUE_GPIO CONFIG_BLUE_GPIO

static void control_led(uint8_t r_led, uint8_t g_led, uint8_t b_led)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(RED_GPIO, r_led);
    gpio_set_level(GREEN_GPIO, g_led);
    gpio_set_level(BLUE_GPIO, b_led);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring LED GPIO pins");
    gpio_reset_pin(RED_GPIO);
    gpio_reset_pin(GREEN_GPIO);
    gpio_reset_pin(BLUE_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(RED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_GPIO, GPIO_MODE_OUTPUT);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        /* Turn LED green for a second to let the user know a succesful broker connection
           is made.
        */
        control_led(0,1,0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        control_led(0,0,0);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "l2mqtt/led/red", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "l2mqtt/led/green", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "l2mqtt/led/blue", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        /* Publish example, may be for later use */
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:{
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        
        /* Messy solution to filter out garbage data
           TODO: could be improved!
        */
        char data_result[event->data_len];    
        char topic_result[event->topic_len];
        sprintf(data_result, "%.*s", event->data_len, event->data);
        sprintf(topic_result, "%.*s", event->topic_len, event->topic);
        int32_t led_status = strcmp(data_result,"OFF");
 
        /* Set LED based on the topic+data */
        if(strcmp(topic_result,"l2mqtt/led/red") == 0)
            gpio_set_level(RED_GPIO, led_status);
        else if(strcmp(topic_result,"l2mqtt/led/green") == 0)
            gpio_set_level(GREEN_GPIO, led_status);
        else if(strcmp(topic_result,"l2mqtt/led/blue") == 0)
           gpio_set_level(BLUE_GPIO, led_status);
        else
            break;
        ESP_LOGI(TAG, "Turning the %.*s LED: %.*s!", event->topic_len, event->topic, event->data_len, event->data);
        break;
    }
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    /* Set LED to yellow, to let the user know a succesful WIFI connection is made, 
       but a broker connection is yet to be made
    */
    control_led(1,1,0);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* Configure LEDs and turn on the blue LED to let the user know the initializing phase started */
    configure_led();
    control_led(0,0,1);

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
}
