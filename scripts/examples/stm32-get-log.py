import json
from time import sleep
from podtp import Podtp

def main():
    with open('config.json', 'r') as file:
        config = json.loads(file.read())
    
    podtp = Podtp(config)
    if podtp.connect():
        sleep(5)
        podtp.disconnect()

if __name__ == '__main__':
    main()