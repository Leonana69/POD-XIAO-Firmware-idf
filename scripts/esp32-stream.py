import json
from podtp import Podtp
import time

def main():
    with open('config.json', 'r') as file:
        config = json.loads(file.read())
    
    podtp = Podtp(config)
    if podtp.connect():
        podtp.start_stream()
        time.sleep(4)
        podtp.disconnect()

if __name__ == '__main__':
    main()