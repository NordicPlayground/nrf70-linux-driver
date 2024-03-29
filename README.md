# nRF70 series Linux SPI reference driver
Linux driver for the nRF70 series Wi-Fi SoC family from Nordic Semiconductor. The driver is based on the [nRF Connect SDK](https://www.nordicsemi.com/Software-and-tools/Software/nRF-Connect-SDK) and is compatible with the [Linux SPI framework](https://www.kernel.org/doc/html/latest/driver-api/spi.html).

This driver is a reference implementation and is not intended for production use. It is provided as-is and is not supported by Nordic Semiconductor ASA. It can be used in any Linux environment to evaluate the nRF70 series Wi-Fi SoC family.

## Supported hardware
The driver supports the following nRF70 series Wi-Fi SoCs:
 - [nRF7002](https://www.nordicsemi.com/Products/nRF7002)
 - [nRF7001](https://www.nordicsemi.com/Products/nRF7001)

## Supported features
The driver supports the following features:
 - Wi-Fi STA mode
 - Wi-Fi 6 (802.11ax) 1x1 operation
 - Dual-band (2.4 GHz and 5 GHz) operation (Depending on the nRF70 series Wi-Fi SoC variant)
 - WPA2-PSK security
 - WPA3-SAE security
 - Wi-Fi RADIO-TEST mode

# Getting started
The driver can be used with any Linux based products, but it is tested on a Raspberry Pi4B running the Ubuntu 22.04 LTS operating system. Support for other Linux distributions is not part of the scope of this project.
## Prerequisites

### Hardware
The following hardware is required to run the driver:
 - Raspberry Pi4B
 - nRF7002 or nRF7001 Evaluation Kit (without Host MCU)
 - An Interposer board to connect the nRF70 series Wi-Fi SoC to the Raspberry Pi4B header.
    >Note: The Interposer board is not included in the nRF7002 or nRF7001 Evaluation Kit and will need to be manufactured separately. The reference board design files for the same can be found [`here`](https://github.com/NordicPlayground/nrf70-linux-driver/tree/main/boards/RPi4B_interposer).
 - A microSD card (at least 32GB) with Ubuntu 22.04 LTS installed

### Software
The following software is required to run the driver:
 - Ubuntu 22.04.03 LTS (64-bit) comes with `5.15.0-1037-raspi` Linux kernel

### Install dependencies
The following dependencies are required to build the driver, install them using `apt`:
 - [`west`](https://docs.zephyrproject.org/latest/guides/west/install.html)
 - [`device-tree-compiler`](https://packages.ubuntu.com/focal/device-tree-compiler)
 - [`build-essential`](https://packages.ubuntu.com/focal/build-essential)
 - [`wpa_supplicant`](https://packages.ubuntu.com/focal/wpa-supplicant)
   - Make sure that the `wpa_supplicant` version is `2.10` or newer
 - [`wpa_passphrase`](https://packages.ubuntu.com/focal/wpa-passphrase)
 - [`iw`](https://packages.ubuntu.com/focal/iw)

# Build and run the driver
## Build the driver
1. To get the latest version of the driver clone the repository using `west`:
    ```bash
    west init -m git@github.com:NordicPlayground/nrf70-linux-driver.git --mr main
   ```

   If you have downloaded a released version of the driver in a zipped format, do the following after unzipping the folder:
   ```bash
    west init -l .
   ```

2. Update west projects
    ```bash
    west update
    ```
3. Build the driver
    ```bash
    make clean all
    ```

### Driver feature configuration

nRF70 driver has feature flags to enable/disable certain features. These flags can be controlled via the command line arguments to the `make` command. The following flags are available:

| Flag | Description | Default |
| --- | --- | --- |
| `MODE` | Supported modes are `STA` and `RADIO-TEST` | `STA` |
| `LOW_POWER` | Enable low power mode | `0` |

#### Examples

1. Build the driver in STA mode with low power enabled
    ```bash
    make clean all MODE=STA LOW_POWER=1
    ```

Ensure that the `nrf_wifi_fmac_sta.ko` and `dts/nrf70_rpi_interposer.dtbo` files are generated.
## Run the driver
1. Load the DTS overlay

   a. Permanent loading: Configure the Raspberry Pi4B to load the DTS overlay at boot time (Recommended for production)
    ```bash
    sudo cp dts/nrf70_rpi_interposer.dtbo /boot/firmware/overlays/
    # Update config.txt
    sudo vim /boot/firmware/config.txt
    # Add below line at the end of the file
    dtoverlay=nrf70_rpi_interposer
    sudo reboot
    ```
   b. Runtime loading: Load the DTS overlay at runtime (Helpful for development)
    ```bash
    sudo dtoverlay dts/nrf70_rpi_interposer.dtbo
    ```

    Note 1: Using this procedure, the DTS overlay must be loaded every time the Raspberry Pi4B is rebooted.

    Note 2: Using this procedure, in the `dmesg` ignore the warnings `OF: overlay: WARNING: memory leak will occur if overlay removed, property: ******`, these are expected as we are dynamically loading the DTS.
2. Load the driver
    ```bash
    sudo insmod nrf_wifi_fmac_sta.ko
    ```

    Note 1: Using this procedure, the driver must be loaded every time the Raspberry Pi4B is rebooted.

    Note 2: Using this procedure, in the `dmesg` ignore the warnings `nrf_wifi_fmac_sta: loading out-of-tree module taints kernel`, these are expected as we are loading a out-of-tree module.

3. Verify that the driver is loaded
    ```bash
    lsmod | grep nrf_wifi_fmac_sta
    ```
4. Verify network interfaces
    ```bash
    ip link show
    ```
    The driver should create a new network interface called `nrf_wifi`.
5. Verify Wi-Fi interface using `iw`
    ```bash
    iw dev nrf_wifi info
    ```
6. Check the kernel log for driver output
    ```bash
    dmesg
    ```

## Connect to a Wi-Fi network
1. Prepare a WPA supplicant configuration file
    ```bash
    wpa_passphrase <SSID> <PASSWORD> > wpa_supplicant.conf
    ```
2. Connect to the Wi-Fi network
    ```bash
    sudo wpa_supplicant -i nrf_wifi -c wpa_supplicant.conf -B
    ```
3. Verify that the Wi-Fi interface is connected
    ```bash
    iw dev nrf_wifi link
    ```
4. Run a DHCP client to get an IP address
    ```bash
    sudo dhclient nrf_wifi
    ```
5. Verify that the Wi-Fi interface has an IP address
    ```bash
    ip addr show nrf_wifi
    ```
6. Verify that the Wi-Fi interface has internet access
    ```bash
    ping -I nrf_wifi google.com
    ```

## Disconnect from a Wi-Fi network
1. Kill the WPA supplicant
    ```bash
    sudo killall wpa_supplicant
    ```
2. Verify that the Wi-Fi interface is disconnected
    ```bash
    iw dev nrf_wifi link
    ```
## Remove the driver
1. Remove the driver
    ```bash
    sudo rmmod nrf_wifi_fmac_sta
    ```
# Debugging
## Run the WPA supplicant in debug mode
1. Stop the WPA supplicant
    ```bash
    sudo killall wpa_supplicant
    ```
2. Run the WPA supplicant in debug mode
    ```bash
    sudo wpa_supplicant -i nrf_wifi -c wpa_supplicant.conf -dd -f /tmp/wpa_supplicant.log
    ```
3. Share the `/tmp/wpa_supplicant.log` file along with configuration file `wpa_supplicant.conf` for debugging purposes.

## Collect `dmesg`
1. Collect the kernel log
    ```bash
    dmesg > dmesg.log
    ```
2. Collect the kernel log from all previous boots to give a complete picture of the system
    ```bash
    cat /var/log/kern.log* > kern.log
    ```

## Collect Sniffer logs

The procedure to collect sniffer logs is outside the scope of this document as it depends heavily on the environment and the sniffer used.

## Driver feature configuration with RADIO-TEST mode with low power enabled

RADIO-TEST mode is used to characterize TX/RX functionalities of RPU with different test sets.

1. To build the driver with RADIO-TEST mode
   ```bash
    make clean all MODE=RADIO-TEST LOW_POWER=1
    ```
   Ensure that the `rf_wifi_fmac_radio_test.ko` and `dts/nrf70_rpi_interposer.dtbo` files are generated.
2. Load the driver
    ```bash
    sudo insmod nrf_wifi_fmac_radio_test.ko
    ```
3. The commands used for radio test can be found [`here`](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/wifi/radio_test/radio_test_subcommands.html).
4. To set different test parameters `debugfs` is used
   
   > For nRF70 Series device  to transmit 10000 packets (TX transmit count) with the required modulation, TX power and channel
     (e.g. 11g, 54 Mbps, channel 11):
   ```bash
   # echo init=11 > /sys/kernel/debug/nrf/wifi/conf
   # echo tx_pkt_tput_mode=0 > /sys/kernel/debug/nrf/wifi/conf
   # echo tx_pkt_rate=54 > /sys/kernel/debug/nrf/wifi/conf
   # echo tx_pkt_len=1024 > /sys/kernel/debug/nrf/wifi/conf
   # echo tx_pkt_gap=100 > /sys/kernel/debug/nrf/wifi/conf
   # echo tx_pkt_num=10000 > /sys/kernel/debug/nrf/wifi/conf
   # echo tx=1 > /sys/kernel/debug/nrf/wifi/conf
   ```
   Once set check the configuration settings in `/sys/kernel/debug/nrf/wifi/conf`. Typically the configuration will be as below
   ```bash
   # cat /sys/kernel/debug/nrf/wifi/conf
   ************* Configured Parameters ***********
   phy_calib_rxdc = 1
   phy_calib_txdc = 1
   phy_calib_txpow = 0
   phy_calib_rxiq = 1
   phy_calib_txiq = 1
   he_ltf = 2
   he_gi = 2
   tx_pkt_tput_mode = 0
   tx_pkt_sgi = 0
   tx_pkt_preamble = 1
   tx_pkt_mcs = 0
   tx_pkt_rate = 54
   tx_pkt_gap = 100
   tx_pkt_num = 10000
   tx_pkt_len = 1024
   tx_power = 30
   ru_tone = 26
   ru_index = 1
   rx_capture_length = 0
   rx_lna_gain = 0
   rx_bb_gain = 0
   tx_tone_freq = 0
   xo_val = 38
   init = 11
   tx = 0
   rx = 0
   tx_pkt_cw = 15
   reg_domain = 00
   bypass_reg_domain = 0
   ```
6. To view the test results
   ```bash
   # cat /sys/kernel/debug/nrf/wifi/stats
   ```
   Typically the results will be as below after running the test.
   
   ```bash
   # cat /sys/kernel/debug/nrf/wifi/stats
   ************* PHY STATS ***********
   rssi_avg = -64 dBm
   ofdm_crc32_pass_cnt=5140
   ofdm_crc32_fail_cnt=3952
   dsss_crc32_pass_cnt=31786
   dsss_crc32_fail_cnt=2265
   ```
# Known issues

1. Target Wake Time (TWT) feature is not supported yet.
   
2. Intermittent issues when running UDP/TCP throughputs:

   - Firmware lockup (on the nRF70 series device)
   - Linux kernel crash
   
3. Sometimes below warning is observed in the `dmesg`

   - `NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!`
