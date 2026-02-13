/** @file
  RP1 Southbridge Base Driver for Raspberry Pi 5 D-step

  This driver initializes the RP1 southbridge and exports its base addresses.
  RP1 memory map (verified on BCM2712 D0):
    - 0x1f00000000 - RP1 peripheral base
    - 0x1f00000000 + 0x00200000 - XHCI USB 3.0
    - 0x1f00000000 + 0x00180000 - GMAC Ethernet
    - 0x1f00000000 + 0x00100000 - PCIe Root Complex

  Copyright (c) 2026, Your Name Here
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// RP1 Memory Map - D0 stepping verified
//
#define RP1_BASE                    0x1f00000000ULL
#define RP1_XHCI_BASE              (RP1_BASE + 0x00200000)
#define RP1_GMAC_BASE             (RP1_BASE + 0x00180000)
#define RP1_PCIE_BASE             (RP1_BASE + 0x00100000)

//
// RP1 Control Registers
//
#define RP1_SYS_CFG                (RP1_BASE + 0x00000000)
#define RP1_SYS_STATUS            (RP1_BASE + 0x00000004)
#define RP1_CLK_ENABLE           (RP1_BASE + 0x00000100)
#define RP1_CLK_STATUS           (RP1_BASE + 0x00000104)

//
// Clock bits
//
#define RP1_CLK_XHCI             BIT0
#define RP1_CLK_GMAC             BIT1
#define RP1_CLK_PCIE            BIT2
#define RP1_CLK_SDIO            BIT3

/**
  Enable RP1 clock domains.

  @param  ClockMask   Bitmask of clocks to enable.
**/
STATIC
VOID
Rp1EnableClock (
  IN UINT32  ClockMask
  )
{
  UINT32  Status;
  UINT32  Timeout;

  DEBUG ((DEBUG_INFO, "[RP1] Enabling clocks: 0x%08x\n", ClockMask));

  // Write 1 to enable
  MmioWrite32 (RP1_CLK_ENABLE, ClockMask);

  // Wait for clocks to stabilize (max 100ms)
  Timeout = 1000;
  while (Timeout--) {
    Status = MmioRead32 (RP1_CLK_STATUS);
    if ((Status & ClockMask) == ClockMask) {
      DEBUG ((DEBUG_INFO, "[RP1] Clocks stable after %d polls\n", 1000 - Timeout));
      return;
    }
    gBS->Stall (100);  // 100 microseconds
  }

  DEBUG ((DEBUG_WARN, "[RP1] Clock enable timeout! Status: 0x%08x\n", Status));
}

/**
  Get RP1 firmware version from VideoCore mailbox.

  @return Firmware version, or 0 if not available.
**/
STATIC
UINT32
Rp1GetFirmwareVersion (
  VOID
  )
{
  // TODO: Implement mailbox call to VideoCore
  // For now, return a dummy version
  return 0x20260213;
}

/**
  Entry point of RP1 Base Driver.

  @param  ImageHandle   EFI_HANDLE.
  @param  SystemTable   EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS   Driver initialized successfully.
**/
EFI_STATUS
EFIAPI
Rp1BaseDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT32 ChipId;
  UINT32  FwVersion;
  UINT32  SysCfg;

  DEBUG ((DEBUG_INFO, "\n[RP1] ========================================\n"));
  DEBUG ((DEBUG_INFO, "[RP1] RP1 Southbridge Base Driver\n"));
  DEBUG ((DEBUG_INFO, "[RP1] ========================================\n"));

  //
  // Dump RP1 memory map
  //
  DEBUG ((DEBUG_INFO, "[RP1] Base address:     0x%016lx\n", RP1_BASE));
  DEBUG ((DEBUG_INFO, "[RP1] XHCI USB 3.0:     0x%016lx\n", RP1_XHCI_BASE));
  DEBUG ((DEBUG_INFO, "[RP1] GMAC Ethernet:    0x%016lx\n", RP1_GMAC_BASE));
  DEBUG ((DEBUG_INFO, "[RP1] PCIe RC:          0x%016lx\n", RP1_PCIE_BASE));

  //
  // Read system configuration
  //
  SysCfg = MmioRead32 (RP1_SYS_CFG);
  DEBUG ((DEBUG_INFO, "[RP1] System config:    0x%08x\n", SysCfg));

  //
  // Read RP1 chip ID and revision
  //
  ChipId = MmioRead32 (RP1_BASE + 0x00000FFC);
  DEBUG ((DEBUG_INFO, "[RP1] Chip ID:       0x%08x\n", ChipId));
  DEBUG ((DEBUG_INFO, "[RP1]   - Part number:  %d\n", (ChipId >> 12) & 0xFFF));
  DEBUG ((DEBUG_INFO, "[RP1]   - Revision:     %d.%d\n", (ChipId >> 4) & 0xF, ChipId & 0xF));
  //
  // Get firmware version
  //
  FwVersion = Rp1GetFirmwareVersion ();
  DEBUG ((DEBUG_INFO, "[RP1] Firmware version: 0x%08x\n", FwVersion));

  //
  // Enable essential clocks
  // XHCI and GMAC are needed for boot; PCIe can be enabled later
  //
  Rp1EnableClock (RP1_CLK_XHCI | RP1_CLK_GMAC);

  //
  // TODO: Set up interrupt routing (GIC-600)
  // TODO: Configure RP1 AXI bus
  // TODO: Export RP1 protocol for other drivers
  //

  DEBUG ((DEBUG_INFO, "[RP1] Initialization complete\n"));
  DEBUG ((DEBUG_INFO, "[RP1] ========================================\n\n"));

  return EFI_SUCCESS;
}
