
KERN_DIR = 

all:
	make -C $(KERN_DIR) M=`pwd` modules 
	$(CROSS_COMPILE)gcc -o sr04_app sr04_app.c
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order  sr04_app

obj-m += sr04_driver.o

