/** @file
 *  GOP driver for Raspberry Pi 5 D-step
 *
 *  Copyright (c) 2026, TW045261
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/DevicePath.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  EFI_DEVICE_PATH_PROTOCOL    End;
} VENDOR_DEVICE_PATH_NODE;

STATIC VENDOR_DEVICE_PATH_NODE mDevicePathNode = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_CALLER_ID_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(sizeof (EFI_DEVICE_PATH_PROTOCOL)),
      (UINT8)((sizeof (EFI_DEVICE_PATH_PROTOCOL)) >> 8)
    }
  }
};

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL mGop;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE mMode;
STATIC EFI_GRAPHICS_OUTPUT_MODE_INFORMATION mModeInfo;

/**
  Dummy QueryMode function.
**/
EFI_STATUS
EFIAPI
DummyQueryMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN UINT32                                ModeNumber,
  OUT UINTN                                *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
  )
{
  if (ModeNumber != 0) {
    return EFI_UNSUPPORTED;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  *Info = AllocateCopyPool (*SizeOfInfo, &mModeInfo);
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Dummy SetMode function.
**/
EFI_STATUS
EFIAPI
DummySetMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN UINT32                        ModeNumber
  )
{
  if (ModeNumber != 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Dummy Blt function.
**/
EFI_STATUS
EFIAPI
DummyBlt (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
  IN UINTN                             SourceX,
  IN UINTN                             SourceY,
  IN UINTN                             DestinationX,
  IN UINTN                             DestinationY,
  IN UINTN                             Width,
  IN UINTN                             Height,
  IN UINTN                             Delta
  )
{
  // 簡單 framebuffer 填色測試
  if (BltOperation == EfiBltVideoFill) {
    UINT32 *Fb = (UINT32 *)(UINTN)mMode.FrameBufferBase;
    UINT32 Color = *(UINT32 *)BltBuffer;
    UINTN  Pixels = mModeInfo.HorizontalResolution * mModeInfo.VerticalResolution;
    UINTN  i;
    
    for (i = 0; i < Pixels; i++) {
      Fb[i] = Color;
    }
  }
  
  return EFI_SUCCESS;
}

/**
  Initialize the Display DXE driver.
**/
EFI_STATUS
EFIAPI
InitializeDisplayDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle = NULL;

  DEBUG ((DEBUG_INFO, "RPi5D DisplayDxe: Initializing\n"));

  // 設定顯示模式 - 1920x1080 32bpp
  mModeInfo.Version = 0;
  mModeInfo.HorizontalResolution = 1920;
  mModeInfo.VerticalResolution = 1080;
  mModeInfo.PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
  mModeInfo.PixelsPerScanLine = 1920;

  // RPi5 D 版 framebuffer 位址
  // 注意：實際位址需要從 DTB 讀取，這裡先用 0x3b000000 測試
  mMode.MaxMode = 1;
  mMode.Mode = 0;
  mMode.Info = &mModeInfo;
  mMode.SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  mMode.FrameBufferBase = 0x3b000000;
  mMode.FrameBufferSize = 1920 * 1080 * 4;

  // 設定 GOP 協議
  mGop.QueryMode = DummyQueryMode;
  mGop.SetMode = DummySetMode;
  mGop.Blt = DummyBlt;
  mGop.Mode = &mMode;

  // 安裝協議
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiGraphicsOutputProtocolGuid, &mGop,
                  &gEfiDevicePathProtocolGuid,     &mDevicePathNode,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RPi5D DisplayDxe: Failed to install GOP: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "RPi5D DisplayDxe: GOP installed - FB=0x%lx, %dx%d\n",
          mMode.FrameBufferBase,
          mModeInfo.HorizontalResolution,
          mModeInfo.VerticalResolution));

  return EFI_SUCCESS;
}
