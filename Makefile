# Variables with default values
KROOT ?= /lib/modules/`uname -r`/build
MODE?= STA
LOW_POWER?= 0
DTS = dts/nrf70_rpi_interposer.dts

# Private portion
DRV_FUNC_NAME =  _wifi_fmac

# If mode != STA throw error
ifeq (, $(filter STA RADIO-TEST, $(MODE)))
$(error Invalid MODE=$(MODE) specified. Valid values are STA, RADIO-TEST)
endif

ifeq ($(MODE), STA)
DRV_MODE_NAME = _sta
else ifeq ($(MODE), RADIO-TEST)
ccflags-y += -DCONFIG_NRF700X_RADIO_TEST
DRV_MODE_NAME = _radio_test
else ifeq ($(MODE), AP)
DRV_MODE_NAME = _ap
endif

ifeq ($(LOW_POWER), 1)
ccflags-y += -DCONFIG_NRF_WIFI_LOW_POWER
endif

OSAL_DIR	= ../nrfxlib/nrf_wifi
ROOT_INC_DIR	= $(shell cd $(PWD); cd $(OSAL_DIR); pwd)
LINUX_SHIM_INC_DIR = $(shell cd $(PWD); pwd)
LINUX_SHIM_DIR = .

# Common Includes
ccflags-y += -I$(LINUX_SHIM_INC_DIR)/inc
ccflags-y += -I$(LINUX_SHIM_INC_DIR)/src/spi/inc
ccflags-y += -I$(ROOT_INC_DIR)/utils/inc
ccflags-y += -I$(ROOT_INC_DIR)/os_if/inc
ccflags-y += -I$(ROOT_INC_DIR)/bus_if/bal/inc
ccflags-y += -I$(ROOT_INC_DIR)/hw_if/hal/inc
ccflags-y += -I$(ROOT_INC_DIR)/hw_if/hal/inc/fw
ccflags-y += -I$(ROOT_INC_DIR)/fw_if/umac_if/inc
ifeq ($(MODE), RADIO-TEST)
ccflags-y += -I$(ROOT_INC_DIR)/fw_if/umac_if/inc/radio_test
else
ccflags-y += -I$(ROOT_INC_DIR)/fw_if/umac_if/inc/default
endif
ccflags-y += -I$(ROOT_INC_DIR)/fw_if/umac_if/inc/fw
ccflags-y += -I$(ROOT_INC_DIR)/bus_if/bus/spi/inc

# Common Objects
OBJS += $(OSAL_DIR)/utils/src/list.o
OBJS += $(OSAL_DIR)/utils/src/queue.o
OBJS += $(OSAL_DIR)/utils/src/util.o
OBJS += $(OSAL_DIR)/os_if/src/osal.o
OBJS += $(OSAL_DIR)/bus_if/bal/src/bal.o
OBJS += $(OSAL_DIR)/hw_if/hal/src/hal_mem.o
OBJS += $(OSAL_DIR)/hw_if/hal/src/hal_reg.o
OBJS += $(OSAL_DIR)/hw_if/hal/src/hal_api.o
OBJS += $(OSAL_DIR)/hw_if/hal/src/hal_interrupt.o
OBJS += $(OSAL_DIR)/hw_if/hal/src/pal.o
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/cmd.o
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/event.o
ifeq ($(MODE), RADIO-TEST)
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/radio_test/fmac_api.o
else
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/default/fmac_api.o
endif
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/fmac_api_common.o

OBJS += $(OSAL_DIR)/hw_if/hal/src/hal_fw_patch_loader.o
OBJS += $(OSAL_DIR)/bus_if/bus/spi/src/spi.o
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/fmac_util.o

ifneq ($(MODE), RADIO-TEST)
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/rx.o
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/tx.o
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/fmac_vif.o
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/fmac_peer.o
endif
ifeq ($(MODE), AP)
OBJS += $(OSAL_DIR)/fw_if/umac_if/src/fmac_ap.o
endif

OBJS += $(LINUX_SHIM_DIR)/src/netdev.o
OBJS += $(LINUX_SHIM_DIR)/src/linux_util.o
OBJS += $(LINUX_SHIM_DIR)/src/main.o
OBJS += $(LINUX_SHIM_DIR)/src/shim.o
ifeq ($(MODE), RADIO-TEST)
OBJS += $(LINUX_SHIM_DIR)/src/debugfs/radio_test.o
else
OBJS += $(LINUX_SHIM_DIR)/src/debugfs/main.o
endif
OBJS += $(LINUX_SHIM_DIR)/src/cfg80211_if.o
OBJS += $(LINUX_SHIM_DIR)/src/wiphy.o
OBJS += $(LINUX_SHIM_DIR)/src/debugfs/stats.o
OBJS += $(LINUX_SHIM_DIR)/src/debugfs/wlan_fmac_ver.o
ifneq ($(MODE), RADIO-TEST)
OBJS += $(LINUX_SHIM_DIR)/src/debugfs/wlan_fmac_twt.o
endif

OBJS += $(LINUX_SHIM_DIR)/src/spi/src/rpu_hw_if.o
OBJS += $(LINUX_SHIM_DIR)/src/spi/src/device.o
OBJS += $(LINUX_SHIM_DIR)/src/spi/src/spi_if.o


# RAW scan feature is mandatory to work with cfg80211
ccflags-y += \
	     -DCONFIG_NRF700X_MAX_TX_TOKENS=10\
	     -DCONFIG_NRF700X_MAX_TX_AGGREGATION=12 \
	     -DCONFIG_NRF700X_RX_NUM_BUFS=63 \
	     -DCONFIG_NRF700X_RX_MAX_DATA_SIZE=1600 \
	     -DCONFIG_WIFI_MGMT_RAW_SCAN_RESULTS \
	     -DCONFIG_NRF700X_ANT_GAIN_2G=0 \
	     -DCONFIG_NRF700X_ANT_GAIN_5G_BAND1=0 \
	     -DCONFIG_NRF700X_ANT_GAIN_5G_BAND2=0 \
	     -DCONFIG_NRF700X_ANT_GAIN_5G_BAND3=0 \
	     -DCONFIG_NRF700X_BAND_2G_LOWER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_2G_UPPER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_1_LOWER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_1_UPPER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_2A_LOWER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_2A_UPPER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_2C_LOWER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_2C_UPPER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_3_LOWER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_3_UPPER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_4_LOWER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_BAND_UNII_4_UPPER_EDGE_BACKOFF=0 \
	     -DCONFIG_NRF700X_REG_DOMAIN=\"00\" \
	     -DCONFIG_NRF700X_TX_MAX_DATA_SIZE=1600 \
	     -DCONFIG_NRF700X_MAX_TX_PENDING_QLEN=48 \
	     -DCONFIG_NRF700X_RPU_PS_IDLE_TIMEOUT_MS=10

ifneq ($(MODE), RADIO-TEST)
ccflags-y += -DCONFIG_NRF700X_DATA_TX
ccflags-y += -DCONFIG_NRF700X_STA_MODE
endif

OBJS += $(OSAL_DIR)/hw_if/hal/src/hpqm.o

# Driver debugging
# Use one of DBG(4)/INF(3)/ERR(1)
ccflags-y += -DCONFIG_WIFI_NRF700X_LOG_LEVEL=3

# Scan only mode, disabled by default
# ccflags-y += -DCONFIG_NRF700X_SCAN_ONLY

ccflags-y += -DCONFIG_NRF_WIFI_BEAMFORMING=1

# shipped tells the build system to not try to build this file
ifeq ($(MODE), RADIO-TEST)
FW_PATCH_BIN=${ROOT_INC_DIR}/fw_bins/radio_test/nrf70.bin
OBJS += ${OSAL_DIR}/fw_bins/radio_test/fw_patch_bin.o
else
FW_PATCH_BIN=${ROOT_INC_DIR}/fw_bins/default/nrf70.bin
OBJS += ${OSAL_DIR}/fw_bins/default/fw_patch_bin.o
ccflags-y += -DCONFIG_NRF700X_SYSTEM_MODE
endif

ccflags-y += -Werror

NAME = nrf$(DRV_FUNC_NAME)$(DRV_MODE_NAME)

obj-m += $(NAME).o

$(NAME)-objs= $(OBJS)

all: dt gen_patch_bin
	@make -C $(KROOT) M=$(PWD) modules
dt:
	dtc -@ -I dts -O dtb -o $(basename $(DTS)).dtbo $(DTS)

gen_patch_bin:
	cd $(shell dirname ${FW_PATCH_BIN}); \
	ld -r -b binary -o fw_patch_bin.o_shipped nrf70.bin; \
	touch .fw_patch_bin.o.cmd

clean:
	@make -C $(KROOT) M=$(PWD) clean
	@find $(OSAL_DIR) -name "*.o*" | xargs rm -f
	rm -f fw_patch_bin.o
