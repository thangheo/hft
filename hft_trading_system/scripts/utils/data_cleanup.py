#!/usr/bin/env python3

import os
import time
import shutil

# data_cleanup.py - Script to clean up old data files

DATA_DIRECTORIES = [
    '/var/log/hft_trading_system/',
    '/data/historical_data/',
    '/data/real_time_data/'
]

# Define the age threshold (e.g., 7 days)
AGE_THRESHOLD = 7 * 24 * 60 * 60  # 7 days in seconds

def cleanup_files():
    current_time = time.time()
    for directory in DATA_DIRECTORIES:
        if os.path.exists(directory):
            for filename in os.listdir(directory):
                file_path = os.path.join(directory, filename)
                if os.path.isfile(file_path):
                    file_age = current_time - os.path.getmtime(file_path)
                    if file_age > AGE_THRESHOLD:
                        os.remove(file_path)
                        print(f"Deleted file: {file_path}")
                elif os.path.isdir(file_path):
                    dir_age = current_time - os.path.getmtime(file_path)
                    if dir_age > AGE_THRESHOLD:
                        shutil.rmtree(file_path)
                        print(f"Deleted directory: {file_path}")

if __name__ == "__main__":
    cleanup_files()
