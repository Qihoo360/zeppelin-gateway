CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable
else
	CXXFLAGS = -O2 -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable -Wno-maybe-uninitialized -Wno-unused-parameter
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ 
endif
OBJECT = zgw_server


SO_PATH = /usr/local/lib
SRC_DIR = ./src
THIRD_DIR = ./third
OUTPUT = ./output

LIB_PATH = -L$(THIRD_DIR)/slash/output/lib/ \
					 -L$(THIRD_DIR)/pink/output/lib/

LIBS = -lpink \
			 -lslash \
			 -lglog \
			 -lpthread

INCLUDE_PATH = -I$(THIRD_DIR)/slash \
							 -I$(THIRD_DIR)/slash/include \
							 -I$(THIRD_DIR)/pink \
							 -I$(THIRD_DIR)/pink/include

.PHONY: all clean


BASE_BOJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.c)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_BOJS))

PINK = $(THIRD_DIR)/pink/output/lib/libpink.a
SLASH = $(THIRD_DIR)/slash/output/lib/libslash.a
GLOG = $(SO_PATH)/libglog.so.0

all: $(OBJECT)
	rm -rf $(OUTPUT)
	mkdir -p $(OUTPUT)
	mkdir -p $(OUTPUT)/bin
	mkdir -p $(OUTPUT)/log
	cp $(OBJECT) $(OUTPUT)/bin/
	rm -rf $(OBJECT)
	@echo "Success, go, go, go..."


$(OBJECT): $(SLASH) $(PINK) $(GLOG) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

$(SLASH):
	make -C $(THIRD_DIR)/slash/

$(PINK):
	make -C $(THIRD_DIR)/pink/

$(GLOG):
	if [ ! -f $(GLOG) ]; then \
		cd $(THIRD_DIR)/glog; \
		autoreconf -ivf; ./configure; make; echo '*' > $(THIRD_DIR)/glog/.gitignore; cp $(THRID_DIR)/glog/.libs/libglog.so.0 $(SO_PATH); \
	fi; 

clean: 
	rm -rf $(OUTPUT)
	rm -f $(SRC_DIR)/*.o
	rm -rf $(OBJECT)

distclean: clean
	make -C $(THIRD_DIR)/slash/ clean
	make -C $(THIRD_DIR)/pink/ clean


