CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable
else
	CXXFLAGS = -O2 -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable -Wno-maybe-uninitialized -Wno-unused-parameter
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ 
endif
OBJECT = zgw_server


SRC_DIR = ./src
LIBZGW_DIR = ./src/libzgw
THIRD_DIR = ./third
OUTPUT = ./output

LIB_PATH = -L$(THIRD_DIR)/slash/output/lib/ \
					 -L$(THIRD_DIR)/pink/output/lib/ \
					 -L$(THIRD_DIR)/glog/.libs \
					 -L$(THIRD_DIR)/libzp/output/lib/

LIBS = -lzp \
			 -lslash \
			 -lpink \
			 -lglog \
			 -lprotobuf \
			 -lcrypto \
			 -lpthread

INCLUDE_PATH = -I$(LIBZGW_DIR)/\
							 -I$(THIRD_DIR)/slash \
							 -I$(THIRD_DIR)/slash/include \
							 -I$(THIRD_DIR)/pink \
							 -I$(THIRD_DIR)/pink/include \
							 -I$(THIRD_DIR)/rapidxml \
							 -I$(THIRD_DIR)/libzp \
							 -I$(THIRD_DIR)/libzp/include \
							 -I$(THIRD_DIR)/glog/src \

.PHONY: all clean


BASE_BOJS := $(wildcard $(LIBZGW_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.c)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_BOJS))

PINK = $(THIRD_DIR)/pink/output/lib/libpink.a
SLASH = $(THIRD_DIR)/slash/output/lib/libslash.a
LIBZP = $(THIRD_DIR)/libzp/output/lib/libzp.a
GLOG = $(THIRD_DIR)/glog/.libs/libglog.so.0

all: $(OBJECT)
	rm -rf $(OUTPUT)
	mkdir -p $(OUTPUT)
	mkdir -p $(OUTPUT)/bin
	mkdir -p $(OUTPUT)/log
	mkdir -p $(OUTPUT)/lib
	cp -r $(GLOG) $(OUTPUT)/lib/
	cp $(OBJECT) $(OUTPUT)/bin/
	cp -r ./conf $(OUTPUT)/
	rm -rf $(OBJECT)
	@echo "Success, go, go, go..."


$(OBJECT): $(SLASH) $(PINK) $(LIBZP) $(GLOG) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

$(SLASH):
	make -C $(THIRD_DIR)/slash/ __PERF=$(__PERF)

$(PINK):
	make -C $(THIRD_DIR)/pink/ __PERF=$(__PERF)

$(LIBZP):
	make -C $(THIRD_DIR)/libzp/ __PERF=$(__PERF)

$(GLOG):
	if [ ! -f $(GLOG) ]; then \
		cd $(THIRD_DIR)/glog; \
		autoreconf -ivf; ./configure; make; echo '*' > .gitignore;\
	fi; 

clean: 
	rm -rf $(OUTPUT)
	rm -f $(SRC_DIR)/*.o
	rm -f $(LIBZGW_DIR)/*.o
	rm -rf $(OBJECT)

distclean: clean
	make -C $(THIRD_DIR)/slash/ clean
	make -C $(THIRD_DIR)/pink/ clean
	make -C $(THIRD_DIR)/libzp/ clean
