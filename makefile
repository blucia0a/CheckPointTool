##############################################################
#
#                   DO NOT EDIT THIS FILE!
#         
##############################################################
PIN_ROOT := /home/vagrant/cvsandbox/pin-2.13-61206-gcc.4.4.7-linux/
# If the tool is built out of the kit, PIN_ROOT must be specified in the make invocation and point to the kit root.
ifdef PIN_ROOT
CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
else
CONFIG_ROOT := ../Config
endif
include $(CONFIG_ROOT)/makefile.config
include makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules

##############################################################
#
#                   DO NOT EDIT THIS FILE!
#
##############################################################
