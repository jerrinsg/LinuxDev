## Introduction

Scull is a simple character device driver for a hypothetical device. 

Kernel header files are required to compile the device driver : `sudo apt-get install linux-headers-$(uname -r)`

After compiling, to attach the module to the kernel : `$ sudo insmod module.ko`

To write data to device : `# echo "hi" > /dev/scull`

To read data from device : `# cat /dev/scull`
