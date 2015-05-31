

insmod /data/monitor_infrastructure/driver/monitor_infrastructure_driver.ko
insmod /data/reliability_governor/module/LTC_module.ko
chmod 666 /dev/monitor

# enable the sensors
#echo 1 > /sys/bus/i2c/drivers/INA231/4-0045/enable
#echo 1 > /sys/bus/i2c/drivers/INA231/4-0040/enable
#echo 1 > /sys/bus/i2c/drivers/INA231/4-0041/enable
#echo 1 > /sys/bus/i2c/drivers/INA231/4-0044/enable
