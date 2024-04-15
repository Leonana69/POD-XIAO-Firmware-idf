from enum import Enum

PODTP_MAX_DATA_LEN = 254

PODTP_START_BYTE_1 = 0xAD
PODTP_START_BYTE_2 = 0x6E

class PodtpType(Enum):
    ACK = 0x1
    COMMAND = 0x2
    LOG = 0x3
    CTRL = 0x4
    ESP32 = 0xE
    BOOT_LOADER = 0xF
    def __eq__(self, __value: object) -> bool:
        if isinstance(__value, int):
            __value = PodtpType(__value)
        return super().__eq__(__value)

class PodtpPort(Enum):
    # ACK
    ERROR = 0x0
    OK = 0x1

    # COMMAND
    RPYT = 0x0
    TAKEOFF = 0x1
    LAND = 0x2
    HOVER = 0x3
    POSITION = 0x4

    # LOG
    STRING = 0x0

    # CTRL
    LOCK = 0x0
    KEEP_ALIVE = 0x1
    
    # ESP32
    ECHO = 0x0
    START_STM32_BOOTLOADER = 0x1
    ENABLE_STM32 = 0x2
    CONFIG_CAMERA = 0x3
    RESET_STREAM_LINK = 0x4

    # BOOT_LOADER
    LOAD_BUFFER = 0x1
    WRITE_FLASH = 0x2

    def __eq__(self, __value: object) -> bool:
        if isinstance(__value, int):
            __value = PodtpPort(__value)
        return super().__eq__(__value)

PACKET_TYPE_NAMES = {
    PodtpType.ACK.value: 'ACK',
    PodtpType.COMMAND.value: 'COMMAND',
    PodtpType.LOG.value: 'LOG',
    PodtpType.ESP32.value: 'ESP32',
    PodtpType.BOOT_LOADER.value: 'BOOT_LOADER'
}

class RawPacket:
    def __init__(self, length = 1) -> None:
        if length > PODTP_MAX_DATA_LEN + 1 or length < 1:
            raise ValueError("Length must be between 1 and PODTP_MAX_DATA_LEN + 1")
        self.length = length
        self.raw = bytearray(PODTP_MAX_DATA_LEN + 1)
    
    def __getitem__(self, key):
        return self.raw[key]

    def __setitem__(self, key, value):
        self.raw[key] = value

    def __len__(self):
        return self.length

    def unpack(self, buffer):
        self.length = buffer[0]
        self.data = buffer[1:]

class PodtpPacket:
    class Header:
        def __init__(self, buffer: bytearray) -> None:
            self.buffer = buffer

        @property
        def type(self):
            return (self.buffer[0] & 0xF0) >> 4
        
        @type.setter
        def type(self, val: int | PodtpType):
            if isinstance(val, PodtpType):
                val = val.value
            self.buffer[0] = (self.buffer[0] & 0x0F) | ((val & 0x0F) << 4)
        
        @property
        def port(self):
            return self.buffer[0] & 0x07
        
        @port.setter
        def port(self, val: int | PodtpPort):
            if isinstance(val, PodtpPort):
                val = val.value
            self.buffer[0] = (self.buffer[0] & 0xF8) | (val & 0x07)

        @property
        def ack(self):
            return (self.buffer[0] & 0x08) >> 3
        
        @ack.setter
        def ack(self, val: int):
            self.buffer[0] = (self.buffer[0] & 0xF7) | ((val & 0x01) << 3)

    class Data:
        def __init__(self, buffer: bytearray) -> None:
            self.buffer = buffer

        def __getitem__(self, key):
            if isinstance(key, slice):
                start = key.start + 1 if key.start is not None else 1
                stop = key.stop + 1 if key.stop is not None else len(self.buffer)
                return self.buffer[start:stop]
            else:
                return self.buffer[key + 1]
        
        def __setitem__(self, key, value):
            if isinstance(key, slice):
                start = key.start + 1 if key.start is not None else 1
                stop = key.stop + 1 if key.stop is not None else len(self.buffer)
                self.buffer[start:stop] = value
            else:
                self.buffer[key + 1] = value

    def __init__(self) -> None:
        self.length = 0
        self.raw = RawPacket()
        self.header = PodtpPacket.Header(self.raw)
        self.data = PodtpPacket.Data(self.raw)

    def set_header(self, type: int | PodtpType, port: int | PodtpPort, ack: int = 0) -> 'PodtpPacket':
        self.header.type = type
        self.header.port = port
        self.header.ack = ack
        self.length = 1
        return self

    def pack(self) -> bytearray:
        buffer = bytearray(2 + 1 + self.length)
        buffer[0] = PODTP_START_BYTE_1
        buffer[1] = PODTP_START_BYTE_2
        buffer[2] = self.length
        buffer[3:3 + self.length] = self.raw[:self.length]
        return buffer
    
    def unpack(self, buffer) -> bool:
        if buffer[0] != PODTP_START_BYTE_1 or buffer[1] != PODTP_START_BYTE_2:
            return False
        self.length = buffer[2]
        self.raw[:self.length] = buffer[3:3 + self.length]
        return True

    def __repr__(self):
        return f'PodtpPacket(length={self.length}, type={PACKET_TYPE_NAMES[self.header.type]}, port={self.header.port})'