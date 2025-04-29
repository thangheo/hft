#!/bin/bash

# backup.sh - Script to backup data and configuration files

BACKUP_DIR="/backups/hft_trading_system"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
BACKUP_FILE="backup_$TIMESTAMP.tar.gz"

echo "Starting backup..."

# Create backup directory if it doesn't exist
sudo mkdir -p $BACKUP_DIR

# Archive and compress data
sudo tar -czf $BACKUP_DIR/$BACKUP_FILE \
    /etc/hft_trading_system/ \
    /data/historical_data/ \
    /data/real_time_data/ \
    /var/log/hft_trading_system/

echo "Backup completed: $BACKUP_DIR/$BACKUP_FILE"
