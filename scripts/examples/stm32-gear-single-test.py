import json
from podtp import Podtp, print_t
import time

def main():
    with open('config.json', 'r') as file:
        config = json.loads(file.read())
    
    podtp = Podtp(config)
    if podtp.connect():
        if not podtp.send_ctrl_lock(False):
            print_t('Failed to unlock control')
        else:
            print_t('Drone unlocked')
            count = 0
            while count < 10:
                print_t(f'Sending setpoint {count}')
                # max speed: 4
                # podtp.send_command_hover(0, 1, 0, 0)
                # podtp.send_command_hover(0, 4, 0, 0)
                podtp.send_command_hover(0, 4, 1, 0)
                time.sleep(0.1)
                count += 1
            
            podtp.send_command_hover(0, 0, 0, 0)
            time.sleep(2)
            
        podtp.disconnect()

if __name__ == '__main__':
    main()