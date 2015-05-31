
#!bin/bash/

make clean

make

adb push LTC /data/reliability_governor/program 
#adb push test /data/reliability_governor/program



