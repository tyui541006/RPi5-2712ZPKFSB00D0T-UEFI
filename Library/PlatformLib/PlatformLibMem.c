/** @file
*
*  Copyright (c) 2026, TW045261
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi/UefiBaseType.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../../Include/Platform/RPi5D.h"

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *Descriptor;

  Descriptor = (ARM_MEMORY_REGION_DESCRIPTOR *)AllocatePool (
                 sizeof (ARM_MEMORY_REGION_DESCRIPTOR) * 4
                 );

  if (Descriptor == NULL) {
    return;
  }

  // System DRAM
  Descriptor[0].PhysicalBase = RPI5D_SYSTEM_MEMORY_BASE;
  Descriptor[0].VirtualBase  = RPI5D_SYSTEM_MEMORY_BASE;
  Descriptor[0].Length       = RPI5D_SYSTEM_MEMORY_SIZE;
  Descriptor[0].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  // Peripheral MMIO
  Descriptor[1].PhysicalBase = RPI5D_PERIPHERAL_BASE;
  Descriptor[1].VirtualBase  = RPI5D_PERIPHERAL_BASE;
  Descriptor[1].Length       = 0x01000000;
  Descriptor[1].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // End of Table
  Descriptor[2].PhysicalBase = 0;
  Descriptor[2].VirtualBase  = 0;
  Descriptor[2].Length       = 0;
  Descriptor[2].Attributes   = 0;

  *VirtualMemoryMap = Descriptor;
}

/**
  Return the current Boot Mode

  @return   Boot mode

**/
BOOLEAN
ArmPlatformIsPrimaryCore (
  IN UINTN  MpId
  )
{
  return TRUE;
}

/**
  Initialize controllers that must be setup in the early stages of the boot.

  @param[in]   MpId             Processor ID

  @return      EFI_SUCCESS      The function completed successfully.

**/
EFI_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  return EFI_SUCCESS;
}

/**
  Return the core position from the MpId

  @param[in]   MpId             Processor ID

  @return      Core position

**/
UINTN
ArmPlatformGetCorePosition (
  IN UINTN  MpId
  )
{
  return 1;
}

/**
  Check if this core is the primary core

  @param[in]   MpId             Processor ID

  @return      TRUE if it's the primary core

  Get the primary core stack base address

  @param[in]   MpId             Processor ID

  @return      Stack base address

**/
UINTN
ArmPlatformGetPrimaryCoreStackBase (
  IN UINTN  MpId
  )
{
  return 0x3000000;
}
