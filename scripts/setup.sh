#!/bin/bash
set -e
sudo insmod driver/tbq_driver.ko
sudo chmod 666 /dev/tbqueue
./daemon/token_bucket_daemon
