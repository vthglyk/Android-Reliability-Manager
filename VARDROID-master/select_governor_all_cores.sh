#!bin/bash


echo $1 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu5/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu6/cpufreq/scaling_governor
echo $1 > /sys/devices/system/cpu/cpu7/cpufreq/scaling_governor

