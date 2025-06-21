/*
 * Copyright (C) 2019 Center for Industry 4.0
 * All Rights Reserved
 *
 * Center_for_Industry_4.0_LICENSE_PLACEHOLDER
 * Desarrolladores: Enrique Germany, Luciano Radrigan
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

int Write_NVS_int(int32_t data, int key)
{
    
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("Storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%d) opening NVS handle!\n", err);
        return -1;
    }
    else
    {
        switch (key)
        {
        case 1:
            err = nvs_set_i32(my_handle, "status", data);
            break;
        case 2:
            err = nvs_set_i32(my_handle, "ID_Protocol", data);
            break;
        case 3:
            err = nvs_set_i32(my_handle, "BMI270_Sampling", data);
            break;
        case 4:
            err = nvs_set_i32(my_handle, "BMI270_Acc_Sens", data);
            break;
        case 5:
            err = nvs_set_i32(my_handle, "BMI270_Gyr_Sens", data);
            break;
        case 6:
            err = nvs_set_i32(my_handle, "BME688_Sampling", data);
            break;
        case 7:
            err = nvs_set_i32(my_handle, "Dis_Time", data);
            break;
        case 8:
            err = nvs_set_i32(my_handle, "Port_TCP", data);
            break;
        case 9:
            err = nvs_set_i32(my_handle, "Port_UDP", data);
            break;
        case 10:
            err = nvs_set_i32(my_handle, "Host_Ip_Addr", data);
            break;

        default:
            printf("ERROR key\n");
            break;
        }
        printf((err != ESP_OK) ? "Failed in NVS!\n" : "Done\n");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        nvs_close(my_handle);
    }
    fflush(stdout);
    return 0;
}

int Read_NVS_int(int32_t *data, int key)
{
    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("Storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%d) opening NVS handle!\n", err);
        return -1;
    }
    else
    {

        switch (key)
        {
        case 1:
            err = nvs_get_i32(my_handle, "status", data);
            break;
        case 2:
            err = nvs_get_i32(my_handle, "ID_Protocol", data);
            break;
        case 3:
            err = nvs_get_i32(my_handle, "BMI270_Sampling", data);
            break;
        case 4:
            err = nvs_get_i32(my_handle, "BMI270_Acc_Sens", data);
            break;
        case 5:
            err = nvs_get_i32(my_handle, "BMI270_Gyr_Sens", data);
            break;
        case 6:
            err = nvs_get_i32(my_handle, "BME688_Sampling", data);
            break;
        case 7:
            err = nvs_get_i32(my_handle, "Dis_Time", data);
            break;
        case 8:
            err = nvs_get_i32(my_handle, "Port_TCP", data);
            break;
        case 9:
            err = nvs_get_i32(my_handle, "Port_UDP", data);
            break;
        case 10:
            err = nvs_get_i32(my_handle, "Host_Ip_Addr", data);
            break;
        default:
            printf("ERROR key\n");
            break;
        }
        switch (err)
        {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            break;
        default:
            printf("Error (%d) reading!\n", err);
        }
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        nvs_close(my_handle);
    }
    fflush(stdout);
    return 0;
}

int Write_NVS_string(const char *data, int key)
{
    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("Storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%d) opening NVS handle!\n", err);
        return -1;
    }
    else
    {

        switch (key)
        {
        case 11:
            err = nvs_set_str(my_handle, "Ssid", data);
            break;
        case 12:
            err = nvs_set_str(my_handle, "Pass", data);
            break;
        default:
            printf("ERROR key\n");
            break;
        }

        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        nvs_close(my_handle);
    }
    fflush(stdout);
    return 0;
}

int Read_NVS_string(char *data, size_t *length, int key)
{
    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("Storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%d) opening NVS handle!\n", err);
        return -1;
    }
    else
    {

        switch (key)
        {
        case 11:
            err = nvs_get_str(my_handle, "Ssid", data, length);
            break;
        case 12:
            err = nvs_get_str(my_handle, "Pass", data, length);
            break;
        default:
            printf("ERROR key\n");
            break;
        }
        switch (err)
        {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            break;
        default:
            printf("Error (%d) reading!\n", err);
        }
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        nvs_close(my_handle);
    }
    fflush(stdout);
    return 0;
}
