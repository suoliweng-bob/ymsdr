
export BUILD_ROOT = $(shell pwd)

#头文件的路径变量
INCLUDE_WXPT =  $(BUILD_ROOT)/mode/wxpt
INCLUDE_ADS	= $(BUILD_ROOT)/mode/ads
INCLUDE_AIS	= $(BUILD_ROOT)/mode/ais
#export INCLUDE_PATH := -I$(BUILD_ROOT)/_include  -I$(BUILD_ROOT)/mode/wxpt/_inc -I$(BUILD_ROOT)/mode/wxpt/sdr -I$(BUILD_ROOT)/mode/wxpt/tinyxml
INCLUDE_PATH := -I$(BUILD_ROOT)/_include
INCLUDE_PATH += -I$(INCLUDE_ADS)/_inc
INCLUDE_PATH += -I$(INCLUDE_AIS)/_inc
INCLUDE_PATH += -I$(INCLUDE_WXPT)/_inc
INCLUDE_PATH += -I$(INCLUDE_WXPT)/sdr
INCLUDE_PATH += -I$(INCLUDE_WXPT)/tinyxml
INCLUDE_PATH += -I$(INCLUDE_WXPT)/demod
INCLUDE_PATH += -I$(INCLUDE_WXPT)/rtaudio
INCLUDE_PATH += -I$(INCLUDE_WXPT)/audio
INCLUDE_PATH += -I$(INCLUDE_WXPT)/process
INCLUDE_PATH += -I$(INCLUDE_WXPT)/modules/modem
INCLUDE_PATH += -I$(INCLUDE_WXPT)/modules/modem/analog
INCLUDE_PATH += -I$(INCLUDE_WXPT)/modules/modem/digital

#INCLUDE_PATH += :$(INCLUDE_WXPT)/demod

export INCLUDE_PATH

BUILD_DIR = $(BUILD_ROOT)/signal/ \
			$(BUILD_ROOT)/proc/   \
			$(BUILD_ROOT)/msic/   \
			$(BUILD_ROOT)/mode/   \
			$(BUILD_ROOT)/mode/ads/  \
			$(BUILD_ROOT)/mode/ais/   \
			$(BUILD_ROOT)/mode/wxpt/   \
			$(BUILD_ROOT)/mode/wxpt/audio/   \
			$(BUILD_ROOT)/mode/wxpt/demod/   \
			$(BUILD_ROOT)/mode/wxpt/modules/modem/   \
			$(BUILD_ROOT)/mode/wxpt/modules/modem/analog/   \
			$(BUILD_ROOT)/mode/wxpt/modules/modem/digital/   \
			$(BUILD_ROOT)/mode/wxpt/process/   \
			$(BUILD_ROOT)/mode/wxpt/rtaudio/   \
			$(BUILD_ROOT)/mode/wxpt/sdr/  \
			$(BUILD_ROOT)/mode/wxpt/tinyxml/  \
			$(BUILD_ROOT)/app/
#			$(BUILD_ROOT)/mode/sdr/   
#			$(BUILD_ROOT)/mode/sdr/external/tinyxml  
#			$(BUILD_ROOT)/mode/sdr/src   
#			$(BUILD_ROOT)/mode/sdr/src/util   


#编译时是否生成调试信息
export DEBUG = false

