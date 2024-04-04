import queue
from typing import Optional
from enum import Enum
from .podtp_packet import PodtpPacket, \
    PODTP_MAX_DATA_LEN, PODTP_START_BYTE_1, PODTP_START_BYTE_2

class RxState(Enum):
    PODTP_STATE_START_1 = 0
    PODTP_STATE_START_2 = 1
    PODTP_STATE_LENGTH = 2
    PODTP_STATE_RAW_DATA = 3

class PodtpParser:
    def __init__(self):
        self.rx_state = RxState.PODTP_STATE_START_1
        self.length = 0
        self.packet = PodtpPacket()
        self.packet_queue = queue.Queue()

    def process(self, data: Optional[bytes]):
        if data is None:
            return
        for byte in data:
            match self.rx_state:
                case RxState.PODTP_STATE_START_1:
                    if byte == PODTP_START_BYTE_1:
                        self.rx_state = RxState.PODTP_STATE_START_2
                case RxState.PODTP_STATE_START_2:
                    if byte == PODTP_START_BYTE_2:
                        self.rx_state = RxState.PODTP_STATE_LENGTH
                    else:
                        self.rx_state = RxState.PODTP_STATE_START_1

                case RxState.PODTP_STATE_LENGTH:
                    self.length = byte
                    self.packet.length = 0
                    if self.length > PODTP_MAX_DATA_LEN or self.length == 0:
                        self.rx_state = RxState.PODTP_STATE_START_1
                    else:
                        self.rx_state = RxState.PODTP_STATE_RAW_DATA
                case RxState.PODTP_STATE_RAW_DATA:
                    self.packet.raw[self.packet.length] = byte
                    self.packet.length += 1
                    if self.packet.length == self.length:
                        self.rx_state = RxState.PODTP_STATE_START_1
                        self.packet_queue.put(self.packet)

    def get_packet(self) -> Optional[PodtpPacket]:
        try:
            return self.packet_queue.get_nowait()
        except queue.Empty:
            return None