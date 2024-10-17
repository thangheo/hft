#!/bin/bash

# cron_jobs_setup.sh - Script to set up cron jobs

echo "Setting up cron jobs..."

# Current user's crontab
CRON_FILE="/tmp/cron_jobs.txt"

# Save existing cron jobs
crontab -l > $CRON_FILE 2>/dev/null || true

# Add new cron jobs
echo "0 2 * * * /usr/bin/python3 /path/to/scripts/utils/data_cleanup.py" >> $CRON_FILE
echo "0 0 * * * /bin/bash /path/to/scripts/utils/log_rotate.sh" >> $CRON_FILE
echo "0 3 * * 0 /bin/bash /path/to/scripts/backup.sh" >> $CRON_FILE

# Install new cron file
crontab $CRON_FILE
rm $CRON_FILE

echo "Cron jobs set up successfully."
