
 # 
 # Author: Pietro Mercati
 #
 # email : pimercat@eng.ucsd.edu
 # 
 # If using this code for research purposes, include 
 # references to the following publications
 # 
 # 1) P.Mercati, A. Bartolini, F. Paterna, T. Rosing and L. Benini; A Linux-governor based 
 #    Dynamic Reliability Manager for android mobile devices. DATE 2014.
 # 2) P.Mercati, A. Bartolini, F. Paterna, L. Benini and T. Rosing; An On-line Reliability 
 #    Emulation Framework. EUC 2014
 # 
 #	This file is part of DRM.
 #      DRM is free software: you can redistribute it and/or modify
 #      it under the terms of the GNU General Public License as published by
 #      the Free Software Foundation, either version 3 of the License, or
 #      (at your option) any later version.
 #
 #      DRM is distributed in the hope that it will be useful,
 #      but WITHOUT ANY WARRANTY; without even the implied warranty of
 #      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #      GNU General Public License for more details.
 #
 #      You should have received a copy of the GNU General Public License
 #      along with DRM.  If not, see <http://www.gnu.org/licenses/>


obj-m := monitor_infrastructure_driver.o

all:
	make ARCH=arm CROSS_COMPILE=/home/sem15f6/toolchains/arm-eabi-4.6/bin/arm-eabi- -C /scratch/sem15f6/linux M=$(PWD) modules

# Path Odroid board
#./prebuilts/gcc/linux-x86/arm/arm-eabi-4.6

clean:
	make -C /scratch/sem15f6/linux M=$(PWD) clean
