import datetime

def print_t(*args, **kwargs):
    # Print with timestamp
    current_time = datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]
    print(f"[{current_time}]", *args, **kwargs)