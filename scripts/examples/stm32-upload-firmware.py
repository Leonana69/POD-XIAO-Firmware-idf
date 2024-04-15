import math, time, json
import struct
import argparse
from tqdm import tqdm

from podtp import Podtp
from podtp import PodtpPacket, PodtpType, PodtpPort
from podtp import print_t
from podtp.podtp_packet import PODTP_MAX_DATA_LEN

FIRMWARE_START_PAGE = 128
BUFFER_PAGE_COUNT = 10
PAGE_SIZE = 1024

class LoadBuffer:
    def __init__(self):
        self.bufferPage = 0
        self.offset = 0

    def size(self):
        return struct.calcsize('<HH')

    def pack(self):
        return struct.pack('<HH', self.bufferPage, self.offset)

class WriteFlash:
    def __init__(self):
        self.bufferPage = 0
        self.flashPage = 0
        self.numPages = 0

    def size(self):
        return struct.calcsize('<HHH')

    def pack(self):
        return struct.pack('<HHH', self.bufferPage, self.flashPage, self.numPages)

def read_bin_file(file_path):
    with open(file_path, 'rb') as file:
        return file.read()

def send_write_flash(podtp: Podtp, flash_page, num_pages) -> bool:
    # print_t(f'Writing flash page {flash_page} with {num_pages} pages')
    packet = PodtpPacket().set_header(PodtpType.BOOT_LOADER,
                                      PodtpPort.WRITE_FLASH, ack=True)
    write_flash = WriteFlash()
    write_flash.bufferPage = 0
    write_flash.flashPage = flash_page + FIRMWARE_START_PAGE
    write_flash.numPages = num_pages

    packet.data[:write_flash.size()] = write_flash.pack()
    packet.length = 1 + write_flash.size()
    if not podtp.send_packet(packet, timeout=10):
        print_t(f'Failed to write flash page {flash_page}')
        return False
    return True

def send_load_buffer(podtp: Podtp, file_path) -> bool:
    binary = read_bin_file(file_path)
    packet = PodtpPacket().set_header(PodtpType.BOOT_LOADER,
                                      PodtpPort.LOAD_BUFFER, ack=True)

    load_buffer = LoadBuffer()

    max_packet_load = PODTP_MAX_DATA_LEN - load_buffer.size() - 1

    total_size = len(binary)
    page_count = math.ceil(total_size / PAGE_SIZE)
    buffer_free_size = BUFFER_PAGE_COUNT * PAGE_SIZE

    print_t(f'Uploading {page_count} pages with {math.ceil(total_size / max_packet_load)} packets...')
    index = 0
    pbar = tqdm(total=total_size, unit='B', unit_scale=True, desc='Uploading')
    while index < total_size:
        page = index // PAGE_SIZE
        offset = index % PAGE_SIZE

        if buffer_free_size == 0:
            if not send_write_flash(podtp, page - 10, 10):
                return False
            buffer_free_size = BUFFER_PAGE_COUNT * PAGE_SIZE

        load_buffer.bufferPage = page % 10
        load_buffer.offset = offset
        packet.data[:load_buffer.size()] = load_buffer.pack()
        # the last packet may not be full
        if index + max_packet_load > total_size:
            packet_load = total_size - index
        else:
            packet_load = max_packet_load

        if packet_load > buffer_free_size:
            packet_load = buffer_free_size

        buffer_free_size -= packet_load
        pbar.update(packet_load)

        # print_t(f'Sent page {page} offset {offset} packet_length {packet.length} packet_load {packet_load}')
        packet.data[load_buffer.size():load_buffer.size() + packet_load] = binary[index:index + packet_load]
        packet.length = 1 + load_buffer.size() + packet_load
        if not podtp.send_packet(packet, timeout=5):
            print_t(f'Upload failed at page {page} offset {offset}')
            return False
        index += packet_load

    if not send_write_flash(podtp, page // 10 * 10, page_count - page // 10 * 10):
        return False
    return True

def start_stm32_bootloader(podtp: Podtp):
    packet = PodtpPacket().set_header(PodtpType.ESP32,
                                      PodtpPort.START_STM32_BOOTLOADER)
    packet.length = 2
    packet.data[0] = 1
    podtp.send_packet(packet)

def start_stm32_firmware(podtp: Podtp):
    packet = PodtpPacket().set_header(PodtpType.ESP32,
                                      PodtpPort.START_STM32_BOOTLOADER)
    packet.length = 2
    packet.data[0] = 0
    podtp.send_packet(packet)

if __name__ == '__main__':
    with open('config.json', 'r') as file:
        config = json.loads(file.read())
    parser = argparse.ArgumentParser(description='Upload firmware to STM32 via ESP32 over WiFi')
    parser.add_argument('file', help='Binary file to upload', type=str)
    
    args = parser.parse_args()
    ip = config['ip']
    port = config['port']
    print_t(f'Uploading {args.file} to {ip}:{port}')

    podtp = Podtp(config)
    if not podtp.connect():
        exit(1)

    podtp.keep_alive = False
    start_stm32_bootloader(podtp)
    time.sleep(1)
    if send_load_buffer(podtp, args.file):
        print_t('Upload [OK]')
        start_stm32_firmware(podtp)
    else:
        print_t('Upload [FAILED]')
    podtp.disconnect()
