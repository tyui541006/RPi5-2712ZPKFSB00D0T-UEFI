# RPi5(2712ZPKFSB00D0T)-UEFI

Raspberry Pi 5(BCM2712 D0)UEFI firmware

## This is an introduction
This is  my first time writing UEFI firmware,and I hope it can correctly boot Windows on my Raspberry Pi 5(D0 BCM2712).I might have very little time to maintain this project sice I am still in school,and this code was written using DeepSeek.

Currently,this firmware is still begin tested(But I cannot testit because my TF card reader is broken)

## Information from DeepSeek
--Options supported by this firmware--
-8GB RAM
-Serial port(PL011,115200)
-GOP display driver(1920x1080)
-RP1 southbridge initialization
-XHCI USB 3.0 skeleton
-ACPI DSDT (Memory + CPU)
-ACPI MADT (GIC-600)
-UEFI Shell
-BDS (Boot Device Selection)

*I cannot personally verift whether this information is accurate*

--Area for Improvement--
Status:
---Not implemented---
-PCIe Root Complex(Required for Windows to detect NVMe SSD)
-GMAC Ethernet(Network debugging,PXE boot)
-RP1 GPIO(Control LEDs,buttons,etc.)
-RP1I2C/SPI(Sensors,expansion boards)
-SD Card Controller(Currently can only load firm from SD Card,cannot read/write in UEFI)
---None---
-Boot Logo(Currently GOP only shows black screen)
-UEFI Menu UI(Default text interface)
--Missing--
-ACPI FADT(Power management.hardware architecture description)
-ACPI GTDT(ARM Generic Timer)
-ACPI SPCR(SerialPort Console Redirection)
--Other--
-XHCI Full Driver(Statu:Skeleton)(Currently only initializes,cannot enumerate devices)
-USB Keyboard/Mouse(Statu:Partial)(XHCI skeleton exists but not fully tested)
--Basic--
-Boot Manager Customization(BDS exists but not customized)
-ACPI Table Optimization(DSDT/MADT present,other missing)


## Build Instructions(Information from DeepSeek)
```bash
# Clone EDK2
git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init
cd ..

# Clone edk2-platforms
gti clone https://github.com/tianocore/edk2-platforms.git

# Copy this platform
cp -r PRi5D-UEFI/* edk2-platforms/Platform/RaspberryPi/RPi5D
# Build
cd edk2
source edksetup.sh
buile -a AARCH64 -t GCC5 -b RELEASE -p Platform/RaspberryPi/RPi5D/RPi5D.dsc

## License
BSD-2-Clause
