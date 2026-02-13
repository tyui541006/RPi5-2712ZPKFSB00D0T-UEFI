/** @file
  RP1 XHCI USB 3.0 Controller Driver for Raspberry Pi 5 D-step

  Copyright (c) 2026, Your Name Here
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/Usb2HostController.h>

//
// RP1 XHCI registers
//
#define RP1_XHCI_BASE              0x1f00200000ULL

// Capability registers
#define XHCI_CAPLENGTH            0x00
#define XHCI_HCIVERSION          0x02
#define XHCI_HCSPARAMS1          0x04
#define XHCI_HCSPARAMS2          0x08
#define XHCI_HCSPARAMS3          0x0C
#define XHCI_HCCPARAMS           0x10
#define XHCI_DBOFF               0x14
#define XHCI_RTSOFF              0x18

// Operational registers
#define XHCI_USBCMD              0x00
#define XHCI_USBSTS             0x04
#define XHCI_PAGESIZE           0x08
#define XHCI_DNCTRL             0x14
#define XHCI_CRCR               0x18
#define XHCI_DCBAAP             0x30
#define XHCI_CONFIG             0x38
#define XHCI_PORTSC             0x400

// Command register bits
#define XHCI_CMD_RUN            BIT0
#define XHCI_CMD_HCRST         BIT1
#define XHCI_CMD_INTE          BIT2
#define XHCI_CMD_HSEE          BIT3
#define XHCI_CMD_LWCR          BIT7

// Status register bits
#define XHCI_STS_HCH           BIT0
#define XHCI_STS_HSE           BIT2
#define XHCI_STS_EINT          BIT3
#define XHCI_STS_PCD           BIT4
#define XHCI_STS_SSS           BIT8
#define XHCI_STS_RSS           BIT9
#define XHCI_STS_SRE           BIT10
#define XHCI_STS_CNR           BIT11
#define XHCI_STS_HCE           BIT12

// Port status register bits
#define XHCI_PORT_CCS          BIT0
#define XHCI_PORT_PED          BIT1
#define XHCI_PORT_OCA          BIT3
#define XHCI_PORT_PR           BIT4
#define XHCI_PORT_PP           BIT9
#define XHCI_PORT_CSC          BIT17
#define XHCI_PORT_PEC          BIT18
#define XHCI_PORT_PRC          BIT21

// Max slots and ports
#define XHCI_GET_MAX_SLOTS(x)  ((x) & 0xFF)
#define XHCI_GET_MAX_PORTS(x)  (((x) >> 24) & 0xFF)

//
// XHCI data structures
//
#pragma pack(1)
typedef struct {
  UINT64                  Address;
  UINT32                  Reserved0;
  UINT32                  Reserved1;
} XHCI_TRB;

typedef struct {
  UINT64                  DeviceContextBaseAddressArray;
  UINT32                  Reserved0;
  UINT32                  Reserved1;
} XHCI_DCBAAP_STRUCT;

typedef struct {
  XHCI_TRB                *RingSegment;
  UINT32                  DequeuePointer;
  UINT32                  CycleState;
} XHCI_COMMAND_RING;
#pragma pack()

//
// Private context for XHCI controller
//
typedef struct {
  UINT32                  Signature;
  EFI_USB2_HC_PROTOCOL    Usb2HcProtocol;
  UINT64                  XhciBase;
  UINT32                  MaxSlots;
  UINT32                  MaxPorts;
  UINT32                  PageSize;
  UINT32                  CapLength;
  VOID                    *Dcbaa;
  XHCI_COMMAND_RING       CommandRing;
} XHCI_PRIVATE_DATA;

#define XHCI_PRIVATE_SIGNATURE  SIGNATURE_32('X', 'H', 'C', 'I')

/**
  Reset the XHCI host controller.

  @param  This          USB2 HC protocol instance.
  @param  Attributes    Reset attributes.

  @retval EFI_SUCCESS   Controller reset successfully.
**/
EFI_STATUS
EFIAPI
XhciReset (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT16                 Attributes
  )
{
  XHCI_PRIVATE_DATA *Private;
  UINT32            Status;
  UINT32            Timeout;

  Private = CR (This, XHCI_PRIVATE_DATA, Usb2HcProtocol, XHCI_PRIVATE_SIGNATURE);
  
  DEBUG ((DEBUG_INFO, "[XHCI] Resetting controller\n"));

  MmioWrite32 (Private->XhciBase + XHCI_USBCMD, XHCI_CMD_HCRST);

  Timeout = 10000;
  do {
    gBS->Stall (100);
    Status = MmioRead32 (Private->XhciBase + XHCI_USBSTS);
    Timeout--;
  } while ((!(Status & XHCI_STS_HCH)) && (Timeout > 0));

  if (Timeout == 0) {
    DEBUG ((DEBUG_ERROR, "[XHCI] Reset timeout!\n"));
    return EFI_TIMEOUT;
  }

  DEBUG ((DEBUG_INFO, "[XHCI] Reset complete\n"));
  return EFI_SUCCESS;
}

/**
  Get the current state of the USB controller.

  @param  This          USB2 HC protocol instance.
  @param  State         Current state.

  @retval EFI_SUCCESS   State retrieved successfully.
**/
EFI_STATUS
EFIAPI
XhciGetState (
  IN  EFI_USB2_HC_PROTOCOL   *This,
  OUT EFI_USB_HC_STATE      *State
  )
{
  XHCI_PRIVATE_DATA *Private;
  UINT32            UsbCmd;
  UINT32            UsbSts;

  Private = CR (This, XHCI_PRIVATE_DATA, Usb2HcProtocol, XHCI_PRIVATE_SIGNATURE);

  UsbCmd = MmioRead32 (Private->XhciBase + XHCI_USBCMD);
  UsbSts = MmioRead32 (Private->XhciBase + XHCI_USBSTS);

  if (UsbCmd & XHCI_CMD_RUN) {
    *State = EfiUsbHcStateOperational;
  } else {
    *State = EfiUsbHcStateHalt;
  }

  return EFI_SUCCESS;
}

/**
  Set the USB controller state.

  @param  This          USB2 HC protocol instance.
  @param  State         New state.

  @retval EFI_SUCCESS   State set successfully.
**/
EFI_STATUS
EFIAPI
XhciSetState (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN EFI_USB_HC_STATE      State
  )
{
  XHCI_PRIVATE_DATA *Private;

  Private = CR (This, XHCI_PRIVATE_DATA, Usb2HcProtocol, XHCI_PRIVATE_SIGNATURE);

  switch (State) {
  case EfiUsbHcStateHalt:
    MmioAnd32 (Private->XhciBase + XHCI_USBCMD, ~XHCI_CMD_RUN);
    break;
  case EfiUsbHcStateOperational:
    MmioOr32 (Private->XhciBase + XHCI_USBCMD, XHCI_CMD_RUN);
    break;
  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Get the root hub port status.

  @param  This          USB2 HC protocol instance.
  @param  PortNumber    Port number (0-based).
  @param  PortStatus    Port status.

  @retval EFI_SUCCESS   Port status retrieved successfully.
**/
EFI_STATUS
EFIAPI
XhciGetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL   *This,
  IN  UINT8                 PortNumber,
  OUT EFI_USB_PORT_STATUS   *PortStatus
  )
{
  XHCI_PRIVATE_DATA *Private;
  UINT32            PortOffset;
  UINT32            PortSc;

  Private = CR (This, XHCI_PRIVATE_DATA, Usb2HcProtocol, XHCI_PRIVATE_SIGNATURE);

  if (PortNumber >= Private->MaxPorts) {
    return EFI_INVALID_PARAMETER;
  }

  PortOffset = XHCI_PORTSC + (PortNumber * 0x10);
  PortSc = MmioRead32 (Private->XhciBase + PortOffset);

  ZeroMem (PortStatus, sizeof (EFI_USB_PORT_STATUS));

  if (PortSc & XHCI_PORT_CCS) {
    PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
  }
  if (PortSc & XHCI_PORT_PED) {
    PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
  }
  if (PortSc & XHCI_PORT_PP) {
    PortStatus->PortStatus |= USB_PORT_STAT_POWER;
  }
  if (PortSc & XHCI_PORT_CSC) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
  }
  if (PortSc & XHCI_PORT_PEC) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
  }
  if (PortSc & XHCI_PORT_PRC) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_RESET;
  }

  return EFI_SUCCESS;
}

/**
  Set the root hub port feature.

  @param  This          USB2 HC protocol instance.
  @param  PortNumber    Port number (0-based).
  @param  Feature       Feature to set.

  @retval EFI_SUCCESS   Feature set successfully.
**/
EFI_STATUS
EFIAPI
XhciSetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  Feature
  )
{
  XHCI_PRIVATE_DATA *Private;
  UINT32            PortOffset;

  Private = CR (This, XHCI_PRIVATE_DATA, Usb2HcProtocol, XHCI_PRIVATE_SIGNATURE);

  if (PortNumber >= Private->MaxPorts) {
    return EFI_INVALID_PARAMETER;
  }

  PortOffset = XHCI_PORTSC + (PortNumber * 0x10);

  switch (Feature) {
  case EfiUsbPortPower:
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_PP);
    break;
  case EfiUsbPortReset:
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_PR);
    break;
  case EfiUsbPortEnable:
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_PED);
    break;
  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Clear the root hub port feature.

  @param  This          USB2 HC protocol instance.
  @param  PortNumber    Port number (0-based).
  @param  Feature       Feature to clear.

  @retval EFI_SUCCESS   Feature cleared successfully.
**/
EFI_STATUS
EFIAPI
XhciClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  Feature
  )
{
  XHCI_PRIVATE_DATA *Private;
  UINT32            PortOffset;

  Private = CR (This, XHCI_PRIVATE_DATA, Usb2HcProtocol, XHCI_PRIVATE_SIGNATURE);

  if (PortNumber >= Private->MaxPorts) {
    return EFI_INVALID_PARAMETER;
  }

  PortOffset = XHCI_PORTSC + (PortNumber * 0x10);

  switch (Feature) {
  case EfiUsbPortPower:
    MmioAnd32 (Private->XhciBase + PortOffset, ~XHCI_PORT_PP);
    break;
  case EfiUsbPortReset:
    MmioAnd32 (Private->XhciBase + PortOffset, ~XHCI_PORT_PR);
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_PRC);
    break;
  case EfiUsbPortEnable:
    MmioAnd32 (Private->XhciBase + PortOffset, ~XHCI_PORT_PED);
    break;
  case EfiUsbPortConnectChange:
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_CSC);
    break;
  case EfiUsbPortEnableChange:
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_PEC);
    break;
  case EfiUsbPortResetChange:
    MmioOr32 (Private->XhciBase + PortOffset, XHCI_PORT_PRC);
    break;
  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Initialize XHCI controller.

  @param  Private       XHCI private data.

  @retval EFI_SUCCESS   Controller initialized successfully.
**/
STATIC
EFI_STATUS
XhciInitController (
  IN XHCI_PRIVATE_DATA  *Private
  )
{
  UINT32  HcParams1;
  UINT32  HcParams2;
  UINT32  HccParams;
  UINT32  Pages;
  UINT32  DcbaaSize;
  UINT64  *Dcbaa;

  DEBUG ((DEBUG_INFO, "[XHCI] Initializing controller\n"));

  Private->CapLength = MmioRead8 (Private->XhciBase + XHCI_CAPLENGTH);
  HcParams1 = MmioRead32 (Private->XhciBase + XHCI_HCSPARAMS1);
  HcParams2 = MmioRead32 (Private->XhciBase + XHCI_HCSPARAMS2);
  HccParams = MmioRead32 (Private->XhciBase + XHCI_HCCPARAMS);

  Private->MaxSlots = XHCI_GET_MAX_SLOTS (HcParams1);
  Private->MaxPorts = XHCI_GET_MAX_PORTS (HcParams1);
  
  DEBUG ((DEBUG_INFO, "[XHCI] Max slots: %d, Max ports: %d\n", 
          Private->MaxSlots, Private->MaxPorts));

  Pages = MmioRead32 (Private->XhciBase + XHCI_PAGESIZE);
  Private->PageSize = 1 << (Pages + 12);
  DEBUG ((DEBUG_INFO, "[XHCI] Page size: 0x%x\n", Private->PageSize));

  DcbaaSize = Private->MaxSlots * sizeof (UINT64);
  Dcbaa = AllocatePages (EFI_SIZE_TO_PAGES (DcbaaSize));
  if (Dcbaa == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem (Dcbaa, DcbaaSize);
  Private->Dcbaa = Dcbaa;

  MmioWrite32 (Private->XhciBase + XHCI_DCBAAP, (UINT32)(UINTN)Dcbaa);
  if (HccParams & BIT0) {
    MmioWrite32 (Private->XhciBase + XHCI_DCBAAP + 4, (UINT32)((UINTN)Dcbaa >> 32));
  }

  MmioWrite32 (Private->XhciBase + XHCI_CONFIG, Private->MaxSlots);

  XhciReset (&Private->Usb2HcProtocol, 0);
  MmioOr32 (Private->XhciBase + XHCI_USBCMD, XHCI_CMD_RUN);

  DEBUG ((DEBUG_INFO, "[XHCI] Controller initialized\n"));
  return EFI_SUCCESS;
}

/**
  Entry point of RP1 XHCI Driver.

  @param  ImageHandle   EFI_HANDLE.
  @param  SystemTable   EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS   Driver initialized successfully.
**/
EFI_STATUS
EFIAPI
Rp1XhciDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  XHCI_PRIVATE_DATA   *Private;
  EFI_HANDLE          Handle;

  DEBUG ((DEBUG_INFO, "\n[XHCI] ========================================\n"));
  DEBUG ((DEBUG_INFO, "[XHCI] RP1 XHCI USB 3.0 Driver\n"));
  DEBUG ((DEBUG_INFO, "[XHCI] ========================================\n"));

  Private = AllocateZeroPool (sizeof (XHCI_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = XHCI_PRIVATE_SIGNATURE;
  Private->XhciBase = RP1_XHCI_BASE;
  DEBUG ((DEBUG_INFO, "[XHCI] Controller base: 0x%016lx\n", Private->XhciBase));

  Private->Usb2HcProtocol.Reset               = XhciReset;
  Private->Usb2HcProtocol.GetState            = XhciGetState;
  Private->Usb2HcProtocol.SetState            = XhciSetState;
  Private->Usb2HcProtocol.ControlTransfer     = NULL;
  Private->Usb2HcProtocol.BulkTransfer        = NULL;
  Private->Usb2HcProtocol.SyncInterruptTransfer  = NULL;
  Private->Usb2HcProtocol.IsochronousTransfer = NULL;
  Private->Usb2HcProtocol.AsyncInterruptTransfer = NULL;
  Private->Usb2HcProtocol.GetRootHubPortStatus = XhciGetRootHubPortStatus;
  Private->Usb2HcProtocol.SetRootHubPortFeature = XhciSetRootHubPortFeature;
  Private->Usb2HcProtocol.ClearRootHubPortFeature = XhciClearRootHubPortFeature;
  Private->Usb2HcProtocol.MajorRevision       = 3;
  Private->Usb2HcProtocol.MinorRevision       = 0;

  Status = XhciInitController (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[XHCI] Controller init failed: %r\n", Status));
    FreePool (Private);
    return Status;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiUsb2HcProtocolGuid, &Private->Usb2HcProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[XHCI] Protocol install failed: %r\n", Status));
    FreePool (Private);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "[XHCI] Driver loaded successfully\n"));
  DEBUG ((DEBUG_INFO, "[XHCI] ========================================\n\n"));

  return EFI_SUCCESS;
}
