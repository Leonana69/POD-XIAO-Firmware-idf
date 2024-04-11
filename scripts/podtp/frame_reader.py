import io, os, time
from enum import Enum
from PIL import Image
from typing import Optional
from threading import Lock
from .utils import print_t
import numpy as np

CAMERA_START_BYTE_1 = 0xAE
CAMERA_START_BYTE_2 = 0x6D

class RxState(Enum):
    IMAGE_STATE_START_1 = 0
    IMAGE_STATE_START_2 = 1
    IMAGE_STATE_LENGTH = 2
    IMAGE_STATE_RAW_DATA = 3

class FrameReader:
    def __init__(self, cache_path: str = 'cache'):
        self.image = None
        self.rx_state = RxState.IMAGE_STATE_START_1
        # 4 bytes for the image size
        self.image_count = 0
        self.image_size_index = None
        self.image_size_bytes = None
        self.image_size = None
        self.image_bytes = None
        self.image_index = None
        self.lock = Lock()
        self.frame = np.array(Image.new('RGB', (320, 240)))
        self.cache_path = cache_path
        self.timestamp = 0

    @property
    def frame(self):
        with self.lock:
            return self._frame
        
    @frame.setter
    def frame(self, value):
        with self.lock:
            self._frame = value

    def process(self, data: Optional[bytes]):
        if data is None:
            return
        
        index = 0
        length = len(data)
        while index < length:
            match self.rx_state:
                case RxState.IMAGE_STATE_START_1:
                    if data[index] == CAMERA_START_BYTE_1:
                        self.rx_state = RxState.IMAGE_STATE_START_2
                    index += 1
                case RxState.IMAGE_STATE_START_2:
                    if data[index] == CAMERA_START_BYTE_2:
                        self.rx_state = RxState.IMAGE_STATE_LENGTH
                        self.image_size_index = 0
                        self.image_size_bytes = bytearray(4)
                    else:
                        self.rx_state = RxState.IMAGE_STATE_START_1
                    index += 1
                case RxState.IMAGE_STATE_LENGTH:
                    self.image_size_bytes[self.image_size_index] = data[index]
                    self.image_size_index += 1
                    if self.image_size_index == 4:
                        self.image_size = int.from_bytes(self.image_size_bytes, 'little')
                        self.timestamp = time.time()
                        self.image_bytes = bytearray(self.image_size)
                        self.image_index = 0
                        self.rx_state = RxState.IMAGE_STATE_RAW_DATA
                    index += 1
                case RxState.IMAGE_STATE_RAW_DATA:
                    # calculate the number of bytes to copy
                    copy_bytes = min(self.image_size - self.image_index, length - index)
                    self.image_bytes[self.image_index:self.image_index + copy_bytes] = data[index:index + copy_bytes]
                    self.image_index += copy_bytes
                    index += copy_bytes
                    if self.image_index == self.image_size:
                        self.frame = np.array(Image.open(io.BytesIO(self.image_bytes)).rotate(180))
                        self.rx_state = RxState.IMAGE_STATE_START_1
                        # duration = time.time() - self.timestamp
                        # print(f'Image size {self.image_size}, time: {duration}, data rate: {self.image_size / (duration):.2f} bytes/s')
                        self.image_count += 1
                    