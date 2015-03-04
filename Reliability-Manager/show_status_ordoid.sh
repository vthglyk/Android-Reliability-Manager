#!bin/bash

echo "Online cores"
cat /sys/devices/system/cpu/cpu0/online
cat /sys/devices/system/cpu/cpu1/online
cat /sys/devices/system/cpu/cpu2/online
cat /sys/devices/system/cpu/cpu3/online
cat /sys/devices/system/cpu/cpu4/online
cat /sys/devices/system/cpu/cpu5/online
cat /sys/devices/system/cpu/cpu6/online
cat /sys/devices/system/cpu/cpu7/online

echo "Operating Frequencies"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu1/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu2/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu5/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu6/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu7/cpufreq/scaling_cur_freq

