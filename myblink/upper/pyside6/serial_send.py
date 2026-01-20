import serial

class SerialError(enumerate):
    SERIAL_ERROR_SUCCESS = 0
    SERIAL_ERROR_NOT_CONNECTED = -1
    SERIAL_ERROR_WRITE_ERROR = -2

class SerialSend():
    def __init__(self):
        self.portName = None
        self.port = None
        self.baudrate = 115200
        self.isConnected = False

    def Connect(self, portName: str, baudrate=115200) -> bool:
        if self.isConnected:
            print(f"串口已经l连接")
            return True

        try:
            self.port = serial.Serial(port=portName, baudrate=baudrate, bytesize=8, parity="N", stopbits=1, timeout=1)
            self.portName = portName
            self.baudrate = baudrate
            self.isConnected = True
            print(f"串口l连接功")
            return True
        except Exception as e:
            print(f"串口 {portName} 连接失败")
            return False

    def DisConnect(self):
        if self.isConnected and self.port.is_open():
            self.port.close()
            self.isConnected = False
            print(f"串口已断开")

    def Send(self, data: bytearray) -> int:
        if not self.isConnected:
            return SerialError.SERIAL_ERROR_NOT_CONNECTED
        try:
            return self.port.write(data)
        except Exception as e:
            return SerialError.SERIAL_ERROR_WRITE_ERROR
