/*
 * DSDT for Raspberry Pi 5 D-step
 * 最簡可用版本
 */

DefinitionBlock ("Dsdt.aml", "DSDT", 2, "RPI5", "RPI5D", 0x00000001)
{
    Name (_HID, "BCM2712")
    Name (_CID, "BCM2712")
    Name (_UID, 0)
    
    // System Memory - 2GB
    Name (MEM0, ResourceTemplate ()
    {
        QWordMemory (
            ResourceConsumer,
            PosDecode,
            MinFixed,
            MaxFixed,
            Cacheable,
            ReadWrite,
            0x00000000,         // 粒度
            0x00000000,         // 最小值
            0x7FFFFFFF,         // 最大值 (2GB-1)
            0x00000000,         // 轉譯
            0x80000000          // 長度 (2GB)
        )
    })
    
    Device (\_SB_.MEM0)
    {
        Name (_HID, "PNP0C01")
        Name (_UID, 0)
        Method (_CRS, 0, Serialized) { Return (MEM0) }
    }
    
    // CPU - 使用 Device 替代 Processor
    Device (\_SB_.CP00)
    {
        Name (_HID, "ACPI0007")
        Name (_UID, 0)
    }
    
    Device (\_SB_.CP01)
    {
        Name (_HID, "ACPI0007")
        Name (_UID, 1)
    }
    
    Device (\_SB_.CP02)
    {
        Name (_HID, "ACPI0007")
        Name (_UID, 2)
    }
    
    Device (\_SB_.CP03)
    {
        Name (_HID, "ACPI0007")
        Name (_UID, 3)
    }
}
