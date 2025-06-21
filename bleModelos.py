from peewee import (
    PostgresqlDatabase,
    Model,
    IntegerField,
    CharField,
    FloatField,
    DateTimeField,
    ForeignKeyField,
    AutoField,
)
import datetime

# Configuración de la base de datos PostgreSQL
db_config = {
    "host": "localhost",      # Host de la base de datos
    "port": 5432,            # Puerto de conexión
    "user": "postgres",       # Usuario de la base de datos
    "password": "postgres",  # Contraseña del usuario
    "database": "iot_db",    # Nombre de la base de datos
}

# Crear instancia de la base de datos
db = PostgresqlDatabase(**db_config)

def set_search_path():
    """Establece el esquema de búsqueda en la base de datos"""
    db.execute_sql("SET search_path TO ble_db;")

class BaseModel(Model):
    """Modelo base que heredarán todos los demás modelos"""
    class Meta:
        database = db  # Asocia todos los modelos con esta base de datos

# Definición de modelos de la base de datos
class Configuration(BaseModel):
    """Modelo para la tabla de configuración de dispositivos"""
    Id_device = CharField(max_length=45, null=True)         # ID del dispositivo
    Status_conf = IntegerField(null=True)                   # Estado de configuración
    Protocol_conf = IntegerField(null=True)                 # Protocolo usado
    Acc_sampling = IntegerField(null=True)                  # Frecuencia de muestreo acelerómetro
    Acc_sensibility = IntegerField(null=True)              # Sensibilidad del acelerómetro
    Gyro_sensibility = IntegerField(null=True)             # Sensibilidad del giroscopio
    BME688_sampling = IntegerField(null=True)              # Frecuencia de muestreo sensor BME688
    Discontinuos_time = IntegerField(null=True)            # Tiempo de operación discontinua
    TCP_PORT = IntegerField(null=True)                     # Puerto TCP
    UDP_port = IntegerField(null=True)                     # Puerto UDP
    Host_ip_addr = IntegerField(null=True)                 # Dirección IP del host
    Ssid = CharField(max_length=45, null=True)             # SSID de red WiFi
    Pass = CharField(max_length=45, null=True)             # Contraseña de red WiFi

class Log(BaseModel):
    """Modelo para la tabla de logs de dispositivos"""
    log_id = AutoField()                                   # ID autoincremental del log
    Id_device = CharField(max_length=45, null=True)        # ID del dispositivo
    Status_report = IntegerField(null=True)                # Estado del reporte
    Protocol_report = IntegerField(null=True)              # Protocolo del reporte
    Battery_Level = IntegerField(null=True)                # Nivel de batería
    Conf_peripheral = IntegerField(null=True)              # Configuración de periféricos
    Time_client = DateTimeField(null=True)                 # Hora del cliente
    Time_server = IntegerField(null=True)                  # Hora del servidor
    configuration_Id_device = ForeignKeyField(
        Configuration, 
        backref="logs", 
        on_delete="NO ACTION", 
        on_update="NO ACTION"
    )  # Relación con la tabla Configuration

class Data_1(BaseModel):
    """Modelo para la primera tabla de datos de sensores"""
    data_1_id = AutoField()                               # ID autoincremental
    Id_device = CharField(max_length=45, null=True)        # ID del dispositivo
    Battery_level = IntegerField(null=True)                # Nivel de batería
    TimeStamp = IntegerField(null=True)                    # Marca de tiempo
    Temperature = IntegerField(null=True)                  # Temperatura
    Press = IntegerField(null=True)                        # Presión
    Hum = IntegerField(null=True)                          # Humedad
    Co = FloatField(null=True)                             # Monóxido de carbono
    RMS = FloatField(null=True)                            # Valor RMS
    Amp_x = FloatField(null=True)                          # Amplitud en eje X
    Freq_x = FloatField(null=True)                         # Frecuencia en eje X
    Amp_y = FloatField(null=True)                          # Amplitud en eje Y
    Freq_y = FloatField(null=True)                         # Frecuencia en eje Y
    Amp_z = FloatField(null=True)                          # Amplitud en eje Z
    Freq_z = FloatField(null=True)                         # Frecuencia en eje Z
    Time_client = DateTimeField(null=True)                 # Hora del cliente
    Log_Id_device = ForeignKeyField(
        Log, 
        backref="data_1", 
        on_delete="NO ACTION", 
        on_update="NO ACTION"
    )  # Relación con la tabla Log

class Data_2(BaseModel):
    """Modelo para la segunda tabla de datos de sensores"""
    Id_device = CharField(max_length=45, null=True)        # ID del dispositivo
    Racc_x = FloatField(null=True)                         # Aceleración rotacional en X
    Racc_y = FloatField(null=True)                         # Aceleración rotacional en Y
    Racc_z = FloatField(null=True)                         # Aceleración rotacional en Z
    Rgyr_x = FloatField(null=True)                         # Giroscopio en X
    Rgyr_y = FloatField(null=True)                         # Giroscopio en Y
    Rgyr_z = FloatField(null=True)                         # Giroscopio en Z
    Time_client = DateTimeField(null=True)                 # Hora del cliente
    Log_Id_device = ForeignKeyField(
        Log, 
        backref="data_2", 
        on_delete="NO ACTION", 
        on_update="NO ACTION"
    )  # Relación con la tabla Log

def add_data_1_to_db(datos_data: list, log_data: list):
    """Función para agregar datos a la tabla Data_1 y Log"""
    try:
        set_search_path()

        # Verifica si existe la configuración del dispositivo, si no, la crea
        config_exists, created = Configuration.get_or_create(Id_device=log_data[7])
        
        with db.atomic():  # Transacción atómica
            # Crear entrada en el log
            log_entry = Log.create(
                Id_device=log_data[0],
                Status_report=log_data[1],
                Protocol_report=log_data[2],
                Battery_Level=log_data[3],
                Conf_peripheral=log_data[4],
                Time_client=log_data[5],
                Time_server=log_data[6],
                configuration_Id_device=log_data[7],
            )
            
            # Crear entrada en Data_1
            Data_1.create(
                Id_device=log_data[0],
                Battery_level=int(datos_data[0]) if datos_data[0] is not None else None,
                TimeStamp=int(datos_data[1]) if datos_data[1] is not None else None,
                Temperature=float(datos_data[2]) if datos_data[2] is not None else None,
                Press=int(datos_data[3]) if datos_data[3] is not None else None,
                Hum=int(datos_data[4]) if datos_data[4] is not None else None,
                Co=float(datos_data[5]) if datos_data[5] is not None else None,
                RMS=float(datos_data[6]) if datos_data[6] is not None else None,
                Amp_x=float(datos_data[7]) if datos_data[7] is not None else None,
                Freq_x=float(datos_data[8]) if datos_data[8] is not None else None,
                Amp_y=float(datos_data[9]) if datos_data[9] is not None else None,
                Freq_y=float(datos_data[10]) if datos_data[10] is not None else None,
                Amp_z=float(datos_data[11]) if datos_data[11] is not None else None,
                Freq_z=float(datos_data[12]) if datos_data[12] is not None else None,
                Time_client=log_data[5],  
                Log_Id_device=log_entry.log_id,
            )
            print("Datos insertados exitosamente")

    except Exception as error:
        print("Error al agregar datos a la base de datos:", error)

def add_data_2_to_db(datos_data: list, log_data: list):
    """Función para agregar datos a la tabla Data_2 y Log"""
    try:
        set_search_path()

        with db.atomic():  # Transacción atómica
            # Crear entrada en el log
            log = Log.create(
                Id_device=log_data[0],
                Status_report=log_data[1],
                Protocol_report=log_data[2],
                Battery_Level=log_data[3],
                Conf_peripheral=log_data[4],
                Time_client=log_data[5],
                Time_server=log_data[6],
                configuration_Id_device=log_data[7],
            )

            # Crear entrada en Data_2
            datos = Data_2.create(
                Id_device=datos_data[0],
                Racc_x=datos_data[1],
                Racc_y=datos_data[2],
                Racc_z=datos_data[3],
                Rgyr_x=datos_data[4],
                Rgyr_y=datos_data[5],
                Rgyr_z=datos_data[6],
                Time_client=datos_data[7],
                Log_Id_device=log.Id_device,
            )

    except Exception as error:
        print("Error al agregar datos a la base de datos:", error)

def get_conf() -> list:
    """Obtiene todas las configuraciones de la base de datos"""
    set_search_path()
    query = Configuration.select()
    values = []
    for row in query:
        for e in row:
            values.append(e)
    values = values[1::]  # Elimina el primer elemento (id interno)
    return values

def update_conf(conf_data: list):
    """Actualiza la configuración en la base de datos"""
    set_search_path()
    query = Configuration.update(
        Id_device=conf_data[0],
        Status_conf=conf_data[1],
        Protocol_conf=conf_data[2],
        Acc_sampling=conf_data[3],
        Acc_sensibility=conf_data[4],
        Gyro_sensibility=conf_data[5],
        BME688_sampling=conf_data[6],
        Discontinuos_time=conf_data[7],
        TCP_PORT=conf_data[8],
        UDP_port=conf_data[9],
        Host_ip_addr=conf_data[10],
        Ssid=conf_data[11],
        Pass=conf_data[12],
    )

def fetch_attribute_values(attribute):
    """
    Obtiene valores de un atributo específico de Data_1 para gráficos.
    Reemplaza valores None por 0.
    """
    set_search_path()
    query = Data_1.select(getattr(Data_1, attribute)).execute()
    values = [getattr(data, attribute) for data in query]
    # Reemplazar valores None por 0
    values = [0 if value is None else value for value in values]

    # Generar datos x como índices de los valores y
    x_values = list(range(len(values)))
    data_dict = {'x': x_values, 'y': values}

    return data_dict

if __name__ == "__main__":
    #Conexión y pruebas básicas
    db.connect()
    set_search_path()

    # Crear las tablas si no existen
    db.create_tables([Configuration, Log, Data_1, Data_2])

    #Obtener configuración de dispositivo
    config = Configuration.get_or_none(Configuration.Id_device == 1)
    if config:
        print(f"Configuración para dispositivo {config.Id_device}: SSID = {config.Ssid}")

    #Obtener valores de atributo para gráficos
    attribute = 'Battery_level'
    attribute_values = fetch_attribute_values(attribute)
    print(f"Valores del atributo '{attribute}': {attribute_values}")

    db.close()