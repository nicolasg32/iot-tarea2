from view import Ui_Dialog
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtCore import QThread, pyqtSignal, QTimer
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from pyqtgraph import PlotWidget
import pyqtgraph as pg
import asyncio
import struct
import sys
from bleak import BleakClient, BleakScanner, BleakError
from datetime import datetime
import threading
from bleModelos import *

def convert_to_128bit_uuid(short_uuid):
    base_uuid = "00000000-0000-1000-8000-00805F9B34FB"
    short_uuid_hex = "{:04X}".format(short_uuid)
    return base_uuid[:4] + short_uuid_hex + base_uuid[8:]

ESP_MAC = "3c:61:05:65:50:96"
CHARACTERISTIC_UUID = convert_to_128bit_uuid(0xFF01)
#data races
db_lock = threading.Lock()

operation_dict = {
    "Configuración por Bluetooth": 0,
    "Configuración vía TCP en BD": 20,
    "Conexión TCP continua": 21,
    "Conexión TCP discontinua": 22,
    "Conexión UDP": 23,
    "BLE continua": 30,
    "BLE discontinua": 31,
}

async def handle_notification(client, sender, value, config):
    print("Recibiendo datos...")
    print("Data: {}".format(value.hex()))
    print("Handle: {}".format(sender))
    print("len_value:" ,len(value))

    unpacked_data = {}

    if len(value) == 17:
        unpacked_data = unpack_protocol_1(value)
        print("Datos desempaquetados protocolo 1:", unpacked_data)
    elif len(value) == 27:
        unpacked_data = unpack_protocol_2(value)
        print("Datos desempaquetados protocolo 2:", unpacked_data)
    elif len(value) == 31:
        unpacked_data = unpack_protocol_3(value)
        print("Datos desempaquetados protocolo 3:", unpacked_data)
    elif len(value) == 55:
        unpacked_data = unpack_protocol_4(value)
        print("Datos desempaquetados protocolo 4:", unpacked_data)

    if unpacked_data:
        log_data = [
            str(unpacked_data['mac']),
            unpacked_data['transport_layer'],
            unpacked_data['id_protocol'],
            unpacked_data['batt'],
            0,  
            datetime.datetime.now(),  #cliente
            unpacked_data['timestamp'],  #server
            1  # configuration_id_device
        ]
        #protocolo 1 
        if len(value) in [17]:
            datos_data = [
                unpacked_data['batt'],
                unpacked_data['timestamp'],
                None,
                None,
                None,
                None,
                None,
                None,
                None,
                None,
                None,
                None,
                None,
            ]
        #protocolo 2
        if len(value) in [27]:
            datos_data = [
                unpacked_data['batt'],
                unpacked_data['timestamp'],
                unpacked_data['temperature'],
                unpacked_data['press'],
                unpacked_data['hum'],
                unpacked_data['co'],
                None,
                None,
                None,
                None,
                None,
                None,
                None,
            ]
        #protocolo 3
        if len(value) in [31]:
            datos_data = [
                unpacked_data['batt'],
                unpacked_data['timestamp'],
                unpacked_data['temperature'],
                unpacked_data['press'],
                unpacked_data['hum'],
                unpacked_data['co'],
                unpacked_data['rms'],
                None,
                None,
                None,
                None,
                None,
                None,
            ]
        #protocolo 4 
        if len(value) == 55:
            datos_data = [
                unpacked_data['batt'],
                unpacked_data['timestamp'],
                unpacked_data['temperature'],
                unpacked_data['press'],
                unpacked_data['hum'],
                unpacked_data['co'],
                unpacked_data['rms'],
                unpacked_data['ampx'],
                unpacked_data['freqx'],
                unpacked_data['ampy'],
                unpacked_data['freqy'],
                unpacked_data['ampz'],
                unpacked_data['freqz'],
            ]
        #printeamos lo que vamos a insertar 
        print("datos_data:", datos_data)
        print("log_data:", log_data)

        #finalmente insertamos los datos  
        add_data_1_to_db(datos_data, log_data)

    try:
        ascii_message = value.decode("ascii")
        print("Notificación recibida (ASCII):", ascii_message)
        if value == b"CONFIG" and config:
            print(config)
            packet=config
            #ahora no creamos el paquete manual si no que se lo pedimos a la base de datos.
            # packet = config_packet_example()
            #packet = get_config_packet_from_db()
            #escribe cuendo 'CONFIG' es recibido
            await client.write_gatt_char(CHARACTERISTIC_UUID, packet)
            print("paquete enviado")

    except UnicodeDecodeError:
        pass

#funciones para desemaquetar la informacion que llega desde la esp
#protocolo 1
def unpack_protocol_1(value):
    header_format = '<H6sBBH'
    body_format = '<BI'

    header_size = struct.calcsize(header_format)
    body_size = struct.calcsize(body_format)

    header = struct.unpack(header_format, value[:header_size])
    body = struct.unpack(body_format, value[header_size:header_size + body_size])

    header_keys = ['id', 'mac', 'transport_layer', 'id_protocol', 'length']
    body_keys = ['batt', 'timestamp']
    header_dict = dict(zip(header_keys, header))
    body_dict = dict(zip(body_keys, body))
    return {**header_dict, **body_dict}


#protocolo 2
def unpack_protocol_2(value):
    header_format = '<H6sBBH'
    body_format = '<BIBIBI'

    header_size = struct.calcsize(header_format)
    body_size = struct.calcsize(body_format)
    
    header = struct.unpack(header_format, value[:header_size])
    body = struct.unpack(body_format, value[header_size:header_size + body_size])

    header_keys = ['id', 'mac', 'transport_layer', 'id_protocol', 'length']
    body_keys = ['batt', 'timestamp', 'temperature', 'press', 'hum', 'co']
    header_dict = dict(zip(header_keys, header))
    body_dict = dict(zip(body_keys, body))

    return {**header_dict, **body_dict}



#protocolo 3
def unpack_protocol_3(value):
    header_format = '<H6sBBH'
    body_format = '<BIBIBIf'

    header_size = struct.calcsize(header_format)
    body_size = struct.calcsize(body_format)

    header = struct.unpack(header_format, value[:header_size])
    body = struct.unpack(body_format, value[header_size:header_size + body_size])

    header_keys = ['id', 'mac', 'transport_layer', 'id_protocol', 'length']
    body_keys = ['batt', 'timestamp', 'temperature', 'press', 'hum', 'co', 'rms']
    header_dict = dict(zip(header_keys, header))
    body_dict = dict(zip(body_keys, body))
    return {**header_dict, **body_dict}




#protocolo 4
def unpack_protocol_4(value):
    header_format = '<H6sBBH'
    body_format = '<BIBIBIfffffff'

    header_size = struct.calcsize(header_format)
    body_size = struct.calcsize(body_format)

    header = struct.unpack(header_format, value[:header_size])
    body = struct.unpack(body_format, value[header_size:header_size + body_size])

    header_keys = ['id', 'mac', 'transport_layer', 'id_protocol', 'length']
    body_keys = ['batt', 'timestamp', 'temperature', 'press', 'hum', 'co','rms', 'ampx', 'freqx', 'ampy', 'freqy', 'ampz', 'freqz']

    header_dict = dict(zip(header_keys, header))
    body_dict = dict(zip(body_keys, body))

    return {**header_dict, **body_dict}




#data
def create_config_packet(
    status,
    id_protocol,
    bmi270_sampling,
    bmi270_acc_sensibility,
    bmi270_gyro_sensibility,
    bme688_sampling,
    discontinuous_time,
    port_tcp,
    port_udp,
    host_ip_addr,
    ssid,
    password,
):
    data = struct.pack(
        "<bbiiiiiiii10s10s",
        status,
        id_protocol,
        bmi270_sampling,
        bmi270_acc_sensibility,
        bmi270_gyro_sensibility,
        bme688_sampling,
        discontinuous_time,
        port_tcp,
        port_udp,
        host_ip_addr,
        ssid.encode("utf-8"),
        password.encode("utf-8"),
    )
    return data



def config_packet_example():
    print("Creando el paquete de configuración...")
    config_packet = create_config_packet(
        status=1,
        id_protocol=4,
        bmi270_sampling=100,
        bmi270_acc_sensibility=4,
        bmi270_gyro_sensibility=250,
        bme688_sampling=2,
        discontinuous_time=60,
        port_tcp=1234,
        port_udp=5678,
        host_ip_addr=19216811,
        ssid="tu_ssid",
        password="tu_passwor",
    )
    print("Paquete de configuración creado: ")
    print("Size: ", sys.getsizeof(config_packet))
    return config_packet


#----------------------------------------------------------------------------------------------------------------  

async def connect_and_subscribe(config=None):
    while True:
        try:
            device = await BleakScanner.find_device_by_address(ESP_MAC, timeout=5.0)
            if device is None:
                print("No se pudo encontrar el dispositivo")
                await asyncio.sleep(5)  
                continue

            async with BleakClient(device) as client:
                print("Conectado al dispositivo")

                await client.start_notify(
                    CHARACTERISTIC_UUID,
                    lambda sender, value: asyncio.create_task(
                        handle_notification(client, sender, value, config)
                    ),
                    timeout=10.0 
                )
                print("Suscrito a las notificaciones")

                while client.is_connected:
                    await asyncio.sleep(1)

        except BleakError as e:
            print(f"Ocurrió un error durante la conexión: {e}")
        except EOFError as e:
            print(f"Ocurrió un error durante la conexión: {e} (eof)")

        finally:
            print("Intentando reconectar...")
            await asyncio.sleep(0.2)  

class Worker(QThread):
    finished = pyqtSignal()

    def __init__(self, config):
        super().__init__()
        self.config = config

    def run(self):
        asyncio.run(connect_and_subscribe(self.config))
        self.finished.emit()


class Controller:

    def __init__(self, parent):
        self.ui = Ui_Dialog()
        self.parent = parent
        self.connect_task = None
        self.worker = None
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.data_cache1 = {}    
        self.data_cache2 = {}
        self.data_cache3 = {}
        self.timer = QTimer(self.parent)  
        self.timer.timeout.connect(self.periodic_update)  
        self.selected_attributes = [None,None,None]
        self.setup_ui()

    def setup_ui(self):
        self.ui.setupUi(self.parent)


    def setSignals(self):
        self.ui.boton_configuracion_3.clicked.connect(self.discover_esp32_devices)
        self.ui.boton_configuracion.clicked.connect(self.send_config)
        self.ui.boton_inicio.clicked.connect(self.start_monitor)
        self.ui.boton_detener.clicked.connect(self.stop_monitor)
        self.ui.selec_plot1.currentIndexChanged.connect(self.update_plot_1)  
        self.ui.selec_plot2.currentIndexChanged.connect(self.update_plot_2)  
        self.ui.selec_plot3.currentIndexChanged.connect(self.update_plot_3)  
        self.ui.boton_graficar.clicked.connect(self.start_periodic_update) 

    def start_periodic_update(self):
        self.timer.start(5000)  #5segundos

    #esta funcion esta enlazada al QTimer , cuando se inicializa la clase- 
    def periodic_update(self):
        
        self.update_plot_1()
        self.update_plot_2()
        self.update_plot_3()
        print("actualizando graficos")


    def update_plot_1(self):

        self.data_cache1 = fetch_attribute_values(self.ui.selec_plot1.currentText())  
        if self.data_cache1:
            self.plot_pyqtgraph(self.ui.plot1, self.data_cache1)

    def update_plot_2(self):

        self.data_cache2 = fetch_attribute_values(self.ui.selec_plot2.currentText())  
        if self.data_cache2:
            self.plot_pyqtgraph(self.ui.plot2, self.data_cache2)

    def update_plot_3(self):

        self.data_cache3 = fetch_attribute_values(self.ui.selec_plot3.currentText())  
        if self.data_cache3:
            self.plot_pyqtgraph(self.ui.plot3, self.data_cache3)


    def plot_pyqtgraph(self, plot_widget, values):
        plot_widget.clear()

        x_values = values['x']
        y_values = values['y']
        print(x_values)
        print(y_values)
        plot_item = plot_widget.addPlot()

        plot_item.plot(x_values, y_values, pen='r')
        plot_item.setLabel('bottom', 'Índices')
        plot_item.setLabel('left', 'Valores')

    def discover_esp32_devices(self):
        """Function that starts BLE search and shows ESP32 devices found in comboBox."""
        try:
            devices = BleakScanner.discover()
            esp32_devices = [device.name for device in devices if "ESP" in device.name]
            self.ui.selec_esp.clear()
            self.ui.selec_esp.addItems(esp32_devices)

        except Exception as e:
            print(f"Error discovering devices: {e}")

    def send_config(self):
        """Function that sends current configuration from user to selected esp."""
        config = self.create_config()
        mode = operation_dict[self.ui.selec_10.currentText()]
        if mode in [0, 30, 31]:  
            try:
                self.worker = Worker(config)
                self.worker.finished.connect(self.on_worker_finished)
                self.worker.start()
            except Exception as e:
                print(f"Error starting worker: {e}")

    def on_worker_finished(self):
        print("Worker has finished")

    def create_config(self):
        """Function that creates config packet."""
        accSamplingTextEdit = int(self.ui.text_acc_sampling.toPlainText())
        accSensitivityTextEdit = int(self.ui.text_acc_sensibity.toPlainText())
        gyroSensitivityTextEdit = int(self.ui.text_gyro_sensibility.toPlainText())
        bme688SamplingTextEdit = int(self.ui.textEdit_18.toPlainText())
        discontinuousTimeTextEdit = int(self.ui.text_disc_time.toPlainText())
        tcpPortTextEdit = int(self.ui.text_tcp_port.toPlainText())
        udpPortTextEdit = int(self.ui.text_udp_port.toPlainText())
        hostIpTextEdit = int(self.ui.text_host_ip.toPlainText())
        ssidTextEdit = self.ui.text_ssid.toPlainText()
        passwordTextEdit = self.ui.text_pass.toPlainText()
        operationModeComboBox = self.ui.selec_10.currentText()
        protocolIdComboBox = int(self.ui.selec_11.currentText())

        packet = create_config_packet(
            operation_dict[operationModeComboBox],
            protocolIdComboBox,
            accSamplingTextEdit,
            accSensitivityTextEdit,
            gyroSensitivityTextEdit,
            bme688SamplingTextEdit,
            discontinuousTimeTextEdit,
            tcpPortTextEdit,
            udpPortTextEdit,
            hostIpTextEdit,
            ssidTextEdit,
            passwordTextEdit,
        )
        return packet

    def start_monitor(self):
        pass

    def stop_monitor(self):
        pass


if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    Dialog = QtWidgets.QDialog()
    cont = Controller(parent=Dialog)
    ui = cont.ui
    ui.setupUi(Dialog)
    Dialog.show()
    cont.setSignals()

    sys.exit(app.exec_())
