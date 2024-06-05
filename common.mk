
#.PHONY:all clean 

ifeq ($(DEBUG),true)
CC = $(CXX) -std=c++11 -g
VERSION = debug
else
CC = $(CXX) -std=c++11
VERSION = release
endif

CFLAGS=-O2 -g -Wall -W `pkg-config --cflags librtlsdr`
LIBS=`pkg-config --libs librtlsdr` -lpthread -lcurl -lliquid -lSoapySDR -lm

#CC = gcc

SRCS = $(wildcard *.cpp)

OBJS = $(SRCS:.cpp=.o)

DEPS = $(SRCS:.cpp=.d)

BIN := $(addprefix $(BUILD_ROOT)/,$(BIN))

LINK_OBJ_DIR = $(BUILD_ROOT)/app/link_obj
DEP_DIR = $(BUILD_ROOT)/app/dep

#-p是递归创建目录
$(shell mkdir -p $(LINK_OBJ_DIR))
$(shell mkdir -p $(DEP_DIR))

OBJS := $(addprefix $(LINK_OBJ_DIR)/,$(OBJS))
DEPS := $(addprefix $(DEP_DIR)/,$(DEPS))

#找到目录中的所有.o文件（编译出来的）
LINK_OBJ = $(wildcard $(LINK_OBJ_DIR)/*.o)
LINK_OBJ += $(OBJS)

all:$(DEPS) $(OBJS) $(BIN)

ifneq ("$(wildcard $(DEPS))","")  
-include $(DEPS)  
endif

#----------------------------------------------------------------1begin------------------
#$(BIN):$(OBJS)
$(BIN):$(LINK_OBJ)
	@echo "------------------------build $(VERSION) mode--------------------------------!!!"

# gcc -o 是生成可执行文件
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

#----------------------------------------------------------------1end-------------------


#----------------------------------------------------------------2begin-----------------
#%.o:%.c
$(LINK_OBJ_DIR)/%.o:%.cpp
#$(CC) -o $@ -c $^
	$(CC) $(INCLUDE_PATH) -o $@ -c $(filter %.cpp,$^)
#----------------------------------------------------------------2end-------------------


#----------------------------------------------------------------3begin-----------------
#%.d:%.c
$(DEP_DIR)/%.d:%.cpp
#gcc -MM $^ > $@
	echo -n $(LINK_OBJ_DIR)/ > $@

#	gcc -I$(INCLUDE_PATH) -MM $^ >> $@
	$(CC) $(INCLUDE_PATH) -MM $^ >> $@

#----------------------------------------------------------------3end------------------





