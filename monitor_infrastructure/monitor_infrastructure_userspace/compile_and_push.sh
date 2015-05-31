
#!bin/bash/

make clean

make

adb push monitor_userspace /data/monitor_infrastructure/userspace
adb push app_manager /data/monitor_infrastructure/userspace




