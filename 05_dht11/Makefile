
KERN_DIR =

all:
	make -C $(KERN_DIR) M=`pwd` modules 
	$(CROSS_COMPILE)gcc -o dht11_app dht11_app.c
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order  dht11_app

obj-m += dht11_driver.o

