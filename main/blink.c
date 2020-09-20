//#include <stdio.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "driver/gpio.h"
//#include "sdkconfig.h"
//#define BLINK_GPIO CONFIG_BLINK_GPIO
//unsigned int hall_sens_read();
//
//void app_main()
//{
//    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
//       muxed to GPIO on reset already, but some default to other
//       functions and need to be switched to GPIO. Consult the
//       Technical Reference for a list of pads and their default
//       functions.)
//    */
//
//	unsigned int val;
//    gpio_pad_select_gpio(BLINK_GPIO);
//    /* Set the GPIO as a push/pull output */
//    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
//    while(1) {
//
//    	val = hall_sens_read();
//
//    	printf("Hall Sensor Value =  % d\n", val);
//        /* Blink off (output low) */
//	printf("Turning off the LED\n");
//        gpio_set_level(BLINK_GPIO, 0);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        /* Blink on (output high) */
//	printf("Turning on the LED\n");
//        gpio_set_level(BLINK_GPIO, 1);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//    }
//}
/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

//*/http_request

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <time.h>
#include <sys/time.h>
#include "lwip/apps/sntp.h"

int hall_sens_read();
void read_hall_sensor();
void sntp();
static void initialize_sntp(void);
static void obtain_time(void);
/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
//#define EXAMPLE_WIFI_SSID "1 Kiss = PWD"
//#define EXAMPLE_WIFI_PASS "ak71208b"

//#define EXAMPLE_WIFI_SSID "shobit"
//#define EXAMPLE_WIFI_PASS "987654321"

#define EXAMPLE_WIFI_SSID "Mahmud_WiFi"
#define EXAMPLE_WIFI_PASS "123456789"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.thingspeak.com"
#define WEB_PORT 80
#define WEB_URL "https://api.thingspeak.com/update.json?api_key=KI8N3OII8F3HLETI&field1="
//#define WEB_URL "/get"

static const char *TAG = "example";
static const char *REQUEST = "GET " WEB_URL" HTTP/1.0\r\n"
    	    "Host: "WEB_SERVER"\r\n"
    	    "User-Agent: esp-idf/1.0 esp32\r\n"
    	    "\r\n";


short int val=0;
bool hall_ready=false;
TaskHandle_t xHallPrintHandle = NULL;
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void http_get_task()
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
    	if(hall_ready)
    	{
    	char QUEST[200] = "";

    	//val = hall_sens_read();
    	char result[10];

    	sntp();
    	//sprintf(result,"%hd",val);
    	itoa(val,result,10);
    	strcpy((char *)QUEST,"GET " WEB_URL);
    	strcat((char *)QUEST, result);
    	strcat((char *)QUEST, " HTTP/1.0\r\n");
    	strcat((char *)QUEST, "Host: "WEB_SERVER"\r\n");
    	strcat((char *)QUEST, "User-Agent: esp-idf/1.0 esp32\r\n");
    	strcat((char *)QUEST, "\r\n");


    	ESP_LOGI(TAG, "\n REQUEST STRING = %s", QUEST);


    	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);


        ESP_LOGI(TAG, "Connected to AP");

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, QUEST, strlen(QUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
        hall_ready= false;
        vTaskSuspend(NULL);
    }
    }
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xTaskCreatePinnedToCore(&read_hall_sensor, "Hall sensor reader", 5000, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&http_get_task, "Hall sensor write", 8900, NULL, 2, &xHallPrintHandle,1);
}
void read_hall_sensor()
{
	ESP_LOGI(TAG, "Hall sensor reader started");
	int add1,index;
	//sntp();
	while(1) {
		add1 = 0;

		for(index=0; index<10; index++)
		{
			add1 += hall_sens_read();
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			if(index==9)
			{
				val = add1/10;
				hall_ready = true;
				vTaskResume(xHallPrintHandle);

			}
		}
	}
}

/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
void sntp()
{
//    ++boot_count;
//    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2019 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    char strftime_buf[64];

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "CET-3", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Tallinn is: %s", strftime_buf);

    // Set timezone to China Standard Time
//    setenv("TZ", "CST-8", 1);
//    tzset();
//    localtime_r(&now, &timeinfo);
//    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
//
//    const int deep_sleep_sec = 10;
//    ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
//    esp_deep_sleep(1000000LL * deep_sleep_sec);
}

static void obtain_time(void)
{
    //ESP_ERROR_CHECK( nvs_flash_init() );
   // initialise_wifi();
//    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
//                        false, true, portMAX_DELAY);
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2019 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

   // ESP_ERROR_CHECK( esp_wifi_stop() );
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}
