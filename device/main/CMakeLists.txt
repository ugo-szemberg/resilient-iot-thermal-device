idf_component_register(SRCS "src/sd_card.c" "src/devices.c" "src/wifi.c" "src/temperature_sensor.c" "src/system.c" "src/main.c"
                       PRIV_REQUIRES esp_wifi nvs_flash esp_adc fatfs
                       INCLUDE_DIRS "include")