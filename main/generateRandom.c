#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "generateRandom.h"

// Al momento de usar las funciones en main(), recordar inicializar la semilla:
// srand(time(NULL)) // Para garantizar que la semilla siempre es diferente

// Función para generar un número aleatorio en un rango dado
float randomInRange(float min, float max)
{
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

uint16_t generateId()
{
    return (uint16_t)(rand() & 0xFFFF); // Mask to ensure it's 16-bit
}

// Función para generar Ampx
float generateAmpx()
{
    return randomInRange(0.0059, 0.12);
}

// Función para generar Freqx
float generateFreqx()
{
    return randomInRange(29.0, 31.0);
}

// Función para generar Ampy
float generateAmpy()
{
    return randomInRange(0.0041, 0.11);
}

// Función para generar Freqy
float generateFreqy()
{
    return randomInRange(59.0, 61.0);
}

// Función para generar Ampz
float generateAmpz()
{
    return randomInRange(0.008, 0.15);
}

// Función para generar Freqz
float generateFreqz()
{
    return randomInRange(89.0, 91.0);
}

// Función para generar el RMS
float generateRMS(float Ampx, float Ampy, float Ampz)
{
    return sqrtf(powf(Ampx, 2) + powf(Ampy, 2) + powf(Ampz, 2));
}

// Función para generar Acc_X, Acc_Y, o Acc_Z
int16_t *generateAcc()
{
    const int size = 2000;
    // Asignar memoria para el array de floats
    int16_t *int16_t_array = (int16_t *)malloc(size * sizeof(int16_t));

    if (int16_t_array == NULL)
    {
        // Manejo de error si malloc falla
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++)
    {
        int16_t_array[i] = randomInRange(-8000, 8000);
    }

    return int16_t_array;
}

// Función para generar Rgyr_X, Rgyr_Y, o Rgyr_Z
float *generateRgyr()
{
    const int size = 2000;
    // Asignar memoria para el array de floats
    float *float_array = (float *)malloc(size * sizeof(float));

    if (float_array == NULL)
    {
        // Manejo de error si malloc falla
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++)
    {
        float_array[i] = randomInRange(-1000, 1000);
    }

    return float_array;
}

// Función para generar la temperatura (Temp)
int generateTemperature()
{
    return rand() % 26 + 5; // Valor aleatorio entre 5 y 30
}

// Función para generar la humedad (Hum)
int generateHumidity()
{
    return rand() % 51 + 30; // Valor aleatorio entre 30 y 80
}

// Función para generar la presión (Pres)
int generatePressure()
{
    return rand() % 201 + 1000; // Valor aleatorio entre 1000 y 1200
}

// Función para generar el nivel de CO (CO)
int generateCO()
{
    return rand() % 171+ 30; // Valor aleatorio entre 30 y 200
}

// Función para generar el nivel de batería
unsigned char generateBatteryLevel()
{
    return rand() % 100 + 1; // Valor aleatorio entre 1 y 100
}

// Ejemplo de uso en main
int main()
{
    // Inicializar la semilla para el generador de números aleatorios
    srand(time(NULL));

    // Generar y usar el array de aceleraciones
    int16_t *accelerations = generateAcc();
    for (int i = 0; i < 2000; i++)
    {
        printf("Acc[%d]: %d\n", i, accelerations[i]);
    }
    free(accelerations); // Liberar la memoria asignada

    // Generar y usar el array de giroscopio
    float *gyroscope = generateRgyr();
    for (int i = 0; i < 2000; i++)
    {
        printf("Rgyr[%d]: %f\n", i, gyroscope[i]);
    }
    free(gyroscope); // Liberar la memoria asignada

    // Generar otros valores y mostrarlos
    float ampx = generateAmpx();
    float ampy = generateAmpy();
    float ampz = generateAmpz();
    float rms = generateRMS(ampx, ampy, ampz);

    printf("Ampx: %f, Ampy: %f, Ampz: %f, RMS: %f\n", ampx, ampy, ampz, rms);

    int temperature = generateTemperature();
    int humidity = generateHumidity();
    int pressure = generatePressure();
    float co = generateCO();
    unsigned char batteryLevel = generateBatteryLevel();

    printf("Temperature: %d, Humidity: %d, Pressure: %d, CO: %f, Battery Level: %u\n",
           temperature, humidity, pressure, co, batteryLevel);

    return 0;
}
