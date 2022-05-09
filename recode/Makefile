KROOT = /lib/modules/$(shell uname -r)/build

MODULES = dependencies/ driver/

obj-y += $(MODULES)

EXTRA_CFLAGS :=
EXTRA_CFLAGS +=	-std=gnu99					\
		-fno-builtin-memset				\
		-Werror						\
		-Wframe-larger-than=400				\
		-Wno-declaration-after-statement		\
		$(INCLUDES)

PLUGIN :=
CFLAGS_MODULE := 

ifeq ($(PLUGIN), TMA)
	CFLAGS_MODULE += -DTMA_MODULE
endif
ifeq ($(PLUGIN), SECURITY)
	CFLAGS_MODULE += -DSECURITY_MODULE
endif

.PHONY: modules modules_install clean insert debug remove reboot security tma


modules:
	@$(MAKE) -w -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

clean:
	@$(MAKE) -C $(KROOT) M=$(PWD) clean
	rm -rf   Module.symvers modules.order

insert:
# 	sudo insmod $(addsuffix *.ko,$(wildcard $(MODULES)))
# 	@echo $(addsuffix *.ko,$(wildcard $(MODULES)))
# 	sudo insmod $(MODULE_NAME).ko
#	sudo insmod dependencies/idt_patcher/idt_patcher.ko
	sudo insmod dependencies/shook/shook.ko
	sudo insmod driver/recode.ko

debug:
	sudo insmod dependencies/shook/shook.ko dyndbg==pmf
	sudo insmod driver/recode.ko dyndbg==pmf

remove:
	sudo rmmod recode
	sudo rmmod shook
#	sudo rmmod idt_patcher
# 	sudo rmmod $(patsubst %.ko,%,$(addsuffix *.ko,$(wildcard $(MODULES))))

reboot: remove insert