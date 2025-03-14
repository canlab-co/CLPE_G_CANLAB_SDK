obj-m += tegra_vnet.o

PWD=$(shell pwd)
MAKEFILE=$(PWD)/Makefile

all: $(MAKEFILE)
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) SRCDIR=$(PWD) modules
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) SRCDIR=$(PWD) modules_install
	echo 'tegra_vnet' | sudo tee -a /etc/modules
	depmod -a

clean: $(MAKEFILE)
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) SRCDIR=$(PWD) clean
	rm -rf /lib/modules/$(shell uname -r)/extra/tegra_vnet.ko
	depmod -a

