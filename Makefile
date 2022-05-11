### Makefile for the Create CanLab CLPE Client API ###

TARGETS := all clean
SUBDIRS := lib main stresstestmain

.PHONY: $(TARGETS) $(SUBDIRS)

$(TARGETS): $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS) 
