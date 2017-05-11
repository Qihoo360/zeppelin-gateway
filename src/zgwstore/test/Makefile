CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable
else
	CXXFLAGS = -O2 -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -Wno-unused-variable -Wno-maybe-uninitialized -Wno-unused-parameter
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ 
endif
OBJECT = main

ifndef SLASH_PATH
SLASH_PATH = $(realpath ../../../third/slash)
endif

ifndef PINK_PATH 
PINK_PATH = $(realpath ../../../third/pink)
endif

ifndef ZP_PATH
ZP_PATH = $(realpath ../../../third/zeppelin-client)
endif

ifndef HIREDIS_PATH
HIREDIS_PATH = $(realpath ../../../third/hiredis)
endif

SRC_DIR = ..

STATIC_LIBS = $(HIREDIS_PATH)/libhiredis.a

DYNAMIC_LIBS = -L$(SLASH_PATH)/slash/lib/ \
					 		 -L$(PINK_PATH)/pink/lib/ \
					 		 -L$(ZP_PATH)/libzp/lib/ \

LIBS = -lzp \
			 -lslash \
			 -lpink \
			 -lprotobuf \
			 -lpthread

INCLUDE_PATH = -I.. \
							 -I$(SLASH_PATH) \
							 -I$(PINK_PATH) \
							 -I$(ZP_PATH) \
							 -I$(HIREDIS_PATH)

.PHONY: all clean


BASE_BOJS += $(wildcard $(SRC_DIR)/*.cc)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.c)
BASE_BOJS += $(wildcard $(SRC_DIR)/*.cpp)
BASE_BOJS += $(wildcard ./*.cc)
OBJS = $(patsubst %.cc,%.o,$(BASE_BOJS))

SLASH = $(SLASH_PATH)/slash/lib/libslash.a
PINK = $(PINK_PATH)/pink/lib/libpink.a
LIBZP = $(ZP_PATH)/libzp/lib/libzp.a
HIREDIS = $(HIREDIS_PATH)/libhiredis.a

all: $(OBJECT)
	@echo "Success, go, go, go..."


$(OBJECT): $(SLASH) $(PINK) $(LIBZP) $(HIREDIS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(STATIC_LIBS) $(DYNAMIC_LIBS) $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) $(VERSION)

$(SLASH):
	make -C $(SLASH_PATH)/slash __PERF=$(__PERF)

$(PINK):
	make -C $(PINK_PATH)/pink __PERF=$(__PERF) SLASH_PATH=$(SLASH_PATH)

$(LIBZP):
	make -C $(ZP_PATH)/libzp __PERF=$(__PERF) SLASH_PATH=$(SLASH_PATH) PINK_PATH=$(PINK_PATH)

$(HIREDIS):
	make -C $(HIREDIS_PATH)

clean: 
	rm -f $(SRC_DIR)/*.o
	rm -f ./*.o
	rm -rf $(OBJECT)

distclean: clean
	make -C $(SLASH_PATH)/slash clean
	make -C $(PINK_PATH)/pink distclean SLASH_PATH=$(SLASH_PATH)
	make -C $(ZP_PATH)/libzp clean SLASH_PATH=$(SLASH_PATH) PINK_PATH=$(PINK_PATH)
	make -C $(HIREDIS_PATH) clean
