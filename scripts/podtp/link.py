import socket
import select
from enum import Enum
from typing import Optional
from .utils import print_t

LINK_MAX_WAIT_TIME = 500

class WifiLink:
    def __init__(self, server_ip: str, server_port: int):
        self.server_ip = server_ip
        self.server_port = server_port
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.settimeout(LINK_MAX_WAIT_TIME / 1000)
        self.client_connected = False

    def connect(self) -> bool:
        try:
            self.client_socket.connect((self.server_ip, self.server_port))
            self.client_connected = True
            print_t(f'Connected to {self.server_ip}:{self.server_port}')
            return True
        except:
            print_t(f'Failed to connect to {self.server_ip}:{self.server_port}')
            return False

    def disconnect(self):
        self.client_socket.close()
        self.client_connected = False
        print_t(f'Disconnected from {self.server_ip}:{self.server_port}')

    def send(self, data: bytearray) -> bool:
        if not self.client_connected:
            print_t(f'Failed to send packet: Not connected to {self.server_ip}:{self.server_port}')
            return False
        self.client_socket.send(data)
        return True

    def receive(self, length = 4096, timeout = 0.1) -> Optional[bytes]:
        """
        Read from the TCP connection until a packet is returned, or a timeout occurs.
        :param timeout: The number of seconds to wait for a complete packet.
        :return: A PodtpPacket if one is successfully received, None otherwise.
        """
        if not self.client_connected:
            print_t(f'Failed to receive packet: Not connected to {self.server_ip}:{self.server_port}')
            return None
        while True:
            readable, _, _ = select.select([self.client_socket], [], [], timeout)
            if not readable:
                return None

            data = self.client_socket.recv(length)
            if not data:
                print_t("Connection closed by the server.")
                return None
            else:
                return data