import argparse
import json
from podtp import Podtp

def main():
    with open('config.json', 'r') as file:
        config = json.loads(file.read())

    parser = argparse.ArgumentParser(description='Upload firmware to STM32 via ESP32 over WiFi')
    parser.add_argument('-d', '--disable', help='Disable STM32', action='store_true')
    args = parser.parse_args()
    
    podtp = Podtp(config)
    if podtp.connect():
        podtp.stm32_enable(args.disable)
        podtp.disconnect()

if __name__ == '__main__':
    main()