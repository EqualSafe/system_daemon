import dbus
import math
from paho.mqtt import client as mqtt
from datetime import datetime
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib
import dbus.service
import dbus.exceptions

BLUEZ_SERVICE_NAME = 'org.bluez'
DBUS_OM_IFACE = 'org.freedesktop.DBus.ObjectManager'
LE_ADVERTISING_MANAGER_IFACE = 'org.bluez.LEAdvertisingManager1'
DBUS_PROP_IFACE = 'org.freedesktop.DBus.Properties'
GATT_MANAGER_IFACE = 'org.bluez.GattManager1'
GATT_SERVICE_IFACE = 'org.bluez.GattService1'
GATT_CHRC_IFACE = 'org.bluez.GattCharacteristic1'
ADAPTER_IFACE = 'org.bluez.Adapter1'
DEVICE_IFACE = 'org.bluez.Device1'

## MQTT
MQTT_BROKER = '127.0.0.1'  # Or your MQTT broker address
MQTT_PORT = 1883  # Default MQTT port
MQTT_RX_TOPIC = 'sys/ble_serial/tx/+'  # Subscribe to TX topics
MQTT_TX_TOPIC = 'sys/ble_serial/rx/'  # Base topic for publishing RX messages

mqtt_client = None
mainloop = None

class InvalidArgsException(dbus.exceptions.DBusException):
    _dbus_error_name = 'org.freedesktop.DBus.Error.InvalidArgs'

class NotSupportedException(dbus.exceptions.DBusException):
    _dbus_error_name = 'org.bluez.Error.NotSupported'

class NotPermittedException(dbus.exceptions.DBusException):
    _dbus_error_name = 'org.bluez.Error.NotPermitted'

class InvalidValueLengthException(dbus.exceptions.DBusException):
    _dbus_error_name = 'org.bluez.Error.InvalidValueLength'

class FailedException(dbus.exceptions.DBusException):
    _dbus_error_name = 'org.bluez.Error.Failed'

class Application(dbus.service.Object):
    def __init__(self, bus):
        self.path = '/com/blegatt'  # Updated application path
        self.services = []
        dbus.service.Object.__init__(self, bus, self.path)

    def get_path(self):
        return dbus.ObjectPath(self.path)

    def add_service(self, service):
        self.services.append(service)

    @dbus.service.method(DBUS_OM_IFACE, out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        response = {}
        print('GetManagedObjects')

        for service in self.services:
            response[service.get_path()] = service.get_properties()
            chrcs = service.get_characteristics()
            for chrc in chrcs:
                response[chrc.get_path()] = chrc.get_properties()
                descs = chrc.get_descriptors()
                for desc in descs:
                    response[desc.get_path()] = desc.get_properties()

        return response

class Service(dbus.service.Object):
    """
    org.bluez.GattService1 interface implementation
    """
    PATH_BASE = '/org/blegatt'

    def __init__(self, bus, index, uuid, primary):
        self.path = self.PATH_BASE + str(index)
        self.bus = bus
        self.uuid = uuid
        self.primary = primary
        self.characteristics = []
        dbus.service.Object.__init__(self, bus, self.path)

    def get_properties(self):
        return {
            GATT_SERVICE_IFACE: {
                'UUID': self.uuid,
                'Primary': self.primary,
                'Characteristics': dbus.Array(
                    self.get_characteristic_paths(),
                    signature='o')
            }
        }

    def get_path(self):
        return dbus.ObjectPath(self.path)

    def add_characteristic(self, characteristic):
        self.characteristics.append(characteristic)

    def get_characteristic_paths(self):
        result = []
        for chrc in self.characteristics:
            result.append(chrc.get_path())
        return result

    def get_characteristics(self):
        return self.characteristics

    @dbus.service.method(DBUS_PROP_IFACE,
                         in_signature='s',
                         out_signature='a{sv}')
    def GetAll(self, interface):
        if interface != GATT_SERVICE_IFACE:
            raise InvalidArgsException()

        return self.get_properties()[GATT_SERVICE_IFACE]

class Characteristic(dbus.service.Object):
    """
    org.bluez.GattCharacteristic1 interface implementation
    """
    def __init__(self, bus, index, uuid, flags, service):
        self.path = service.path + '/char' + str(index)
        self.bus = bus
        self.uuid = uuid
        self.service = service
        self.flags = flags
        self.descriptors = []
        dbus.service.Object.__init__(self, bus, self.path)

    def get_properties(self):
        return {
            GATT_CHRC_IFACE: {
                'Service': self.service.get_path(),
                'UUID': self.uuid,
                'Flags': self.flags,
                'Descriptors': dbus.Array(
                        self.get_descriptor_paths(),
                        signature='o')
            }
        }

    def get_path(self):
        return dbus.ObjectPath(self.path)

    def add_descriptor(self, descriptor):
        self.descriptors.append(descriptor)

    def get_descriptor_paths(self):
        result = []
        for desc in self.descriptors:
            result.append(desc.get_path())
        return result

    def get_descriptors(self):
        return self.descriptors

    @dbus.service.method(DBUS_PROP_IFACE,
                         in_signature='s',
                         out_signature='a{sv}')
    def GetAll(self, interface):
        if interface != GATT_CHRC_IFACE:
            raise InvalidArgsException()

        return self.get_properties()[GATT_CHRC_IFACE]

    @dbus.service.method(GATT_CHRC_IFACE,
                        in_signature='a{sv}',
                        out_signature='ay')
    def ReadValue(self, options):
        print('Default ReadValue called, returning error')
        raise NotSupportedException()

    @dbus.service.method(GATT_CHRC_IFACE, in_signature='aya{sv}')
    def WriteValue(self, value, options):
        print('Default WriteValue called, returning error')
        raise NotSupportedException()

    @dbus.service.method(GATT_CHRC_IFACE)
    def StartNotify(self):
        print('Default StartNotify called, returning error')
        raise NotSupportedException()

    @dbus.service.method(GATT_CHRC_IFACE)
    def StopNotify(self):
        print('Default StopNotify called, returning error')
        raise NotSupportedException()

    @dbus.service.signal(DBUS_PROP_IFACE,
                         signature='sa{sv}as')
    def PropertiesChanged(self, interface, changed, invalidated):
        pass


class TxCharacteristic(Characteristic):
    def __init__(self, bus, index, service):
        Characteristic.__init__(self, bus, index, "12345678-1234-5678-1234-000000000001", ['notify'], service)
        self.notifying = False

    def ReadValue(self, options):
        return super().ReadValue(options)

    def send_update(self, new_value):
        if not self.notifying:
            print("No subscribers for notifications.")
            return
        self.value = dbus.ByteArray(new_value)
        self.PropertiesChanged(GATT_CHRC_IFACE, {'Value': self.value}, [])

    def StartNotify(self):
        if self.notifying:
            print("Already notifying.")
            return
        self.notifying = True

    def StopNotify(self):
        self.notifying = False

class RxCharacteristic(Characteristic):
    def __init__(self, bus, index, service):
        Characteristic.__init__(self, bus, index, "12345678-1234-5678-1234-000000000002", ['write'], service)

    @dbus.service.method(GATT_CHRC_IFACE, in_signature='aya{sv}', out_signature='')
    def WriteValue(self, value, options):
        timestamp = datetime.now().timestamp()
        topic = MQTT_TX_TOPIC + str(math.floor(timestamp))
        print(f"[{str(math.floor(timestamp))}] BLE <-> MQTT : {topic} {bytes(value).decode('utf-8')}")
        message = bytes(value).decode('utf-8')
        mqtt_client.publish(topic, message)

def find_adapter(bus):
    remote_om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/'),
                               DBUS_OM_IFACE)
    objects = remote_om.GetManagedObjects()

    for o, props in objects.items():
        if GATT_MANAGER_IFACE in props.keys():
            return o

    return None

def register_app_cb():
    print("GATT serial application registered")

def register_app_error_cb(error):
    print(f"Failed to register application: {error}")
    mainloop.quit()

def on_connect(client, userdata, flags, rc, prop):
    print("Connected to MQTT Broker:", MQTT_BROKER)
    mqtt_client.subscribe(MQTT_RX_TOPIC)

def on_message(client, userdata, msg):
    timestamp = datetime.now().timestamp()
    print(f"[{str(math.floor(timestamp))}] MQTT <-> BLE : {msg.topic} {str(msg.payload.decode('utf-8'))}")

    if ble_tx:
        ble_tx.send_update(msg.payload)

if __name__ == '__main__':
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="ble_serial", clean_session=True, reconnect_on_failure=True, protocol= mqtt.MQTTv31)

    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)

    DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    adapter = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/org/bluez/hci0'), ADAPTER_IFACE)
    adapter_props = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, '/org/bluez/hci0'), DBUS_PROP_IFACE)
    adapter_props.Set(ADAPTER_IFACE, 'Powered', dbus.Boolean(1))
    ## additional settings
    adapter_props.Set(ADAPTER_IFACE, 'Pairable', dbus.Boolean(1))
    adapter_props.Set(ADAPTER_IFACE, 'Discoverable', dbus.Boolean(1))
    adapter_props.Set(ADAPTER_IFACE, 'DiscoverableTimeout', dbus.UInt32(0))

    app = Application(bus)
    serial_service = Service(bus, 0, "12345678-1234-5678-1234-000000000000", True)
    app.add_service(serial_service)

    ble_tx = TxCharacteristic(bus, 0, serial_service)
    ble_rx = RxCharacteristic(bus, 1, serial_service)
    serial_service.add_characteristic(ble_tx)
    serial_service.add_characteristic(ble_rx)

    adapter = find_adapter(bus)
    if not adapter:
        print('GattManager1 interface not found')

    obj_manager = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, adapter),GATT_MANAGER_IFACE)
    obj_manager.RegisterApplication(app.get_path(), {},
                                    reply_handler=register_app_cb,
                                    error_handler=register_app_error_cb)

    mqtt_client.loop_start()
    mainloop = GLib.MainLoop()
    mainloop.run()