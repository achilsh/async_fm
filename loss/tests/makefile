CXX = g++
ROOT=.

SUBDIRS=$(shell find $(ROOT) -name '*_test' -type d)
.PHONY : subdirs  
subdirs: 
	@for dir in $(SUBDIRS); do echo "making in $$dir"; ( cd $$dir && make -f makefile -e CC='$(CXX)' CFLAG='$(CFLAG)') || exit 1; done

clean:  
	@for dir in $(SUBDIRS); do echo "cleaning in $$dir"; ( cd $$dir && make clean -f makefile) || exit 1; done

