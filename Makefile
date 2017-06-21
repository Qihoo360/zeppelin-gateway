CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable
else
	CXXFLAGS = -O2 -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable -Wno-maybe-uninitialized -Wno-unused-parameter
endif
OBJECT = zgw_server

SLASH_PATH = $(realpath ./third/slash)
PINK_PATH = $(realpath ./third/pink)
ZP_PATH = $(realpath ./third/zeppelin-client/libzp)
GLOG_PATH = $(realpath ./third/glog)
HIREDIS_PATH = $(realpath ./third/hiredis)
RAPID_XML_PATH = $(realpath ./third/rapidxml)

SRC_DIR = ./src
ZGWSTORE_DIR = ./src/zgwstore
S3CMDS_DIR = ./src/s3_cmds
OUTPUT = ./output
VERSION = -D_GITVER_=$(shell git rev-list HEAD | head -n1) \
					-D_COMPILEDATE_=$(shell date +%F)

LIB_PATH = -L$(SLASH_PATH)/slash/lib \
					 -L$(PINK_PATH)/pink/lib \
					 -L$(GLOG_PATH)/.libs \
					 -L$(ZP_PATH)/libzp/lib

LIBS = -lzp \
			 -lpink \
			 -lslash \
			 -lprotobuf \
			 -lcrypto \
			 -lssl \
			 -lglog \
			 -lpthread

INCLUDE_PATH = -I. \
							 -I$(SLASH_PATH) \
							 -I$(PINK_PATH) \
							 -I$(RAPID_XML_PATH) \
							 -I$(ZP_PATH) \
							 -I$(HIREDIS_PATH) \
							 -I$(GLOG_PATH)/src

.PHONY: all clean


BASE_BOJS := $(wildcard $(ZGWSTORE_DIR)/*.cc)
BASE_BOJS += $(wildcard $(S3CMDS_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.c)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_BOJS))

PINK = $(PINK_PATH)/pink/lib/libpink.a
SLASH = $(SLASH_PATH)/slash/lib/libslash.a
LIBZP = $(ZP_PATH)/libzp/lib/libzp.a
HIREDIS_STATIC = $(HIREDIS_PATH)/libhiredis.a
GLOG = $(GLOG_PATH)/.libs/libglog.a

all: $(OBJECT)
	rm -rf $(OUTPUT)
	mkdir -p $(OUTPUT)/bin
	mkdir -p $(OUTPUT)/log
	cp $(OBJECT) $(OUTPUT)/bin/
	cp -r ./conf $(OUTPUT)/
	rm -rf $(OBJECT)
	@echo "Success, go, go, go..."


$(OBJECT): $(SLASH) $(PINK) $(LIBZP) $(GLOG) $(HIREDIS_STATIC) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS) $(HIREDIS_STATIC)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) $(VERSION)

$(SLASH):
	make -C $(SLASH_PATH)/slash __PERF=$(__PERF)

$(PINK):
	make -C $(PINK_PATH)/pink __PERF=$(__PERF) SLASH_PATH=$(SLASH_PATH)

$(LIBZP):
	make -C $(ZP_PATH)/libzp __PERF=$(__PERF) SLASH_PATH=$(SLASH_PATH) PINK_PATH=$(PINK_PATH)

$(GLOG):
	if [ ! -f $(GLOG) ]; then \
		cd $(GLOG_PATH); \
		autoreconf -ivf; ./configure; make; echo '*' > .gitignore;\
	fi; 

$(HIREDIS_STATIC):
	make -C $(HIREDIS_PATH)

clean: 
	rm -rf $(OUTPUT)
	rm -f $(SRC_DIR)/*.o
	rm -f $(ZGWSTORE_DIR)/*.o
	rm -f $(S3CMDS_DIR)/*.o
	rm -rf $(OBJECT)

distclean: clean
	make -C $(SLASH_PATH)/slash clean
	make -C $(PINK_PATH)/pink distclean SLASH_PATH=$(SLASH_PATH)
	make -C $(ZP_PATH)/libzp clean SLASH_PATH=$(SLASH_PATH) PINK_PATH=$(PINK_PATH)
