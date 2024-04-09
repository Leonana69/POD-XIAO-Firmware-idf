import json
from podtp import Podtp
import time
import numpy as np
import cv2

def main():
    with open('config.json', 'r') as file:
        config = json.loads(file.read())
    
    podtp = Podtp(config)
    if podtp.connect():
        podtp.start_stream()
        frame_reader = podtp.frame_reader
        while True:
            frame = frame_reader.frame
            if frame is not None:
                frame = np.array(frame)
                cv2_image = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                cv2.imshow('frame', cv2_image)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        podtp.disconnect()

if __name__ == '__main__':
    main()