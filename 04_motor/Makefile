
KERN_DIR =

all:
	make -C $(KERN_DIR) M=`pwd` modules 
	$(CROSS_COMPILE)gcc -o motor_app motor_app.c
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order  motor_app

obj-m += motor_driver.o

