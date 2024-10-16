#!/bin/bash

# log_rotate.sh - Script to rotate log files

LOG_DIR="/var/log/hft_trading_system"
MAX_LOG_FILES=5

cd $LOG_DIR

# Rotate logs
if [ -f "trading.log" ]; then
    # Remove the oldest log file
    if [ -f "trading.log.$MAX_LOG_FILES" ]; then
        rm "trading.log.$MAX_LOG_FILES"
    fi

    # Shift log files
    for ((i=$MAX_LOG_FILES-1; i>=1; i--)); do
        if [ -f "trading.log.$i" ]; then
            mv "trading.log.$i" "trading.log.$((i+1))"
        fi
    done

    # Rename current log to trading.log.1
    mv "trading.log" "trading.log.1"
    echo "Rotated logs."
else
    echo "No log file to rotate."
fi

# Create new log file
touch trading.log
