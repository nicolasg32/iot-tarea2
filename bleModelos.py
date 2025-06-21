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

#database
db_config = {
    "host": "localhost",
    "port": 5432,
    "user": "postgres",
    "password": "postgres",
    "database": "iot_db",
}

db = PostgresqlDatabase(**db_config)

def set_search_path():
    db.execute_sql("SET search_path TO ble_db;")

class BaseModel(Model):
    class Meta:
        database = db

#modelos
class Configuration(BaseModel):
    Id_device = CharField(max_length=45, null=True)
    Status_conf = IntegerField(null=True)
    Protocol_conf = IntegerField(null=True)
    Acc_sampling = IntegerField(null=True)
    Acc_sensibility = IntegerField(null=True)
    Gyro_sensibility = IntegerField(null=True)
    BME688_sampling = IntegerField(null=True)
    Discontinuos_time = IntegerField(null=True)
    TCP_PORT = IntegerField(null=True)
    UDP_port = IntegerField(null=True)
    Host_ip_addr = IntegerField(null=True)
    Ssid = CharField(max_length=45, null=True)
    Pass = CharField(max_length=45, null=True)

class Log(BaseModel):
    log_id = AutoField()
    Id_device = CharField(max_length=45, null=True)
    Status_report = IntegerField(null=True)
    Protocol_report = IntegerField(null=True)
    Battery_Level = IntegerField(null=True)
    Conf_peripheral = IntegerField(null=True)
    Time_client = DateTimeField(null=True)
    Time_server = IntegerField(null=True)
    configuration_Id_device = ForeignKeyField(
        Configuration, backref="logs", on_delete="NO ACTION", on_update="NO ACTION"
    )






class Data_1(BaseModel):
    data_1_id = AutoField()
    Id_device = CharField(max_length=45, null=True)
    Battery_level = IntegerField(null=True)
    TimeStamp = IntegerField(null=True)
    Temperature = IntegerField(null=True)
    Press = IntegerField(null=True)
    Hum = IntegerField(null=True)
    Co = FloatField(null=True)
    RMS = FloatField(null=True)
    Amp_x = FloatField(null=True)
    Freq_x = FloatField(null=True)
    Amp_y = FloatField(null=True)
    Freq_y = FloatField(null=True)
    Amp_z = FloatField(null=True)
    Freq_z = FloatField(null=True)
    Time_client = DateTimeField(null=True)
    Log_Id_device = ForeignKeyField(
        Log, backref="data_1", on_delete="NO ACTION", on_update="NO ACTION"

    )








class Data_2(BaseModel):
    Id_device = CharField(max_length=45, null=True)
    Racc_x = FloatField(null=True)
    Racc_y = FloatField(null=True)
    Racc_z = FloatField(null=True)
    Rgyr_x = FloatField(null=True)
    Rgyr_y = FloatField(null=True)
    Rgyr_z = FloatField(null=True)
    Time_client = DateTimeField(null=True)
    Log_Id_device = ForeignKeyField(
        Log, backref="data_2", on_delete="NO ACTION", on_update="NO ACTION"
    )















def add_data_1_to_db(datos_data: list, log_data: list):
    try:
        set_search_path()

        #configuration_Id_device existe, sino, crea una
        config_exists, created = Configuration.get_or_create(Id_device=log_data[7])
        with db.atomic():
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
            print("Data inserted successfully")

    except Exception as error:
        print("Error adding data to the database:", error)














def add_data_2_to_db(datos_data: list, log_data: list):
    try:
        set_search_path()

        with db.atomic():
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
        print("Error adding data to the database:", error)








def get_conf() -> list:
    set_search_path()
    query = Configuration.select()
    values = []
    for row in query:
        for e in row:
            values.append(e)
    values = values[1::]  
    return values










def update_conf(conf_data: list):
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




#funcion para hacer las consultas para los graficos , cuando exiten valores none los cambia por 0. 
def fetch_attribute_values(attribute):
    set_search_path()
    query = Data_1.select(getattr(Data_1, attribute)).execute()
    values = [getattr(data, attribute) for data in query]
    #reemplazar valores None por 0
    values = [0 if value is None else value for value in values]

    #generar datos x como índices de los valores y
    x_values = list(range(len(values)))
    data_dict = {'x': x_values, 'y': values}

    return data_dict


if __name__ == "__main__":
    db.connect()
    set_search_path()

    #crea las tablas
    db.create_tables([Configuration, Log, Data_1, Data_2])

    #test
    config = Configuration.get_or_none(Configuration.Id_device == 1)
    if config:
        print(f"Configuration for device {config.Id_device}: SSID = {config.Ssid}")


    attribute = 'Battery_level'
    attribute_values = fetch_attribute_values(attribute)
    print(f"Values of '{attribute}' attribute: {attribute_values}")

    db.close()
