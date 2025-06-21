// Acceloremeter_kpi

// Función para generar un número aleatorio en un rango dado
float randomInRange(float min, float max);

// Función para generar id
uint16_t generateId();

// Función para generar Ampx
float generateAmpx();

// Función para generar Freqx
float generateFreqx();

// Función para generar Ampy
float generateAmpy();

// Función para generar Freqy
float generateFreqy();

// Función para generar Ampz
float generateAmpz();

// Función para generar Freqz
float generateFreqz();

// Función para generar el RMS
float generateRMS(float Ampx, float Ampy, float Ampz);

// Acceloremeter_Sensor

// Función para generar Acc_X, Acc_Y, o Acc_Z
int16_t *generateAcc();

// Función para generar Rgyr_X, Rgyr_Y, o Rgyr_Z
float *generateRgyr();

// Temperatura-Humedad-Presión-CO

// Función para generar la temperatura (Temp)
int generateTemperature();

// Función para generar la humedad (Hum)
int generateHumidity();

// Función para generar la presión (Pres)
int generatePressure();

// Función para generar el nivel de CO (CO)
int generateCO();

// Batt_Sensor

// Función para generar el nivel de batería
unsigned char generateBatteryLevel();