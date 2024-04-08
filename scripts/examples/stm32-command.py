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
            while count < 3:
                print_t(f'Sending setpoint {count}')
                # podtp.send_command_hover(0.5, 0, 0, 0)
                podtp.send_command_setpoint(0, 0, 0, 11000)
                time.sleep(0.1)
                count += 1
            
        podtp.disconnect()

if __name__ == '__main__':
    main()