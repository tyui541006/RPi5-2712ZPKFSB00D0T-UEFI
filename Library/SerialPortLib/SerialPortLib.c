#include <Base.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include "../../Include/Platform/RPi5D.h"

#define UART_DR     0x000
#define UART_FR     0x018
#define UART_IBRD   0x024
#define UART_FBRD   0x028
#define UART_LCRH   0x02C
#define UART_CR     0x030
#define UART_IMSC   0x038
#define UART_ICR    0x044

#define FR_TXFF     (1 << 5)
#define FR_RXFE     (1 << 4)
#define FR_BUSY     (1 << 3)

/**
  Initialize the serial device hardware.
  
  @retval RETURN_SUCCESS        The serial device was initialized.
**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINT32  Divisor;

  MmioWrite32 (RPI5D_UART_BASE + UART_CR, 0x00000000);
  
  Divisor = 48000000 / (16 * 115200);
  MmioWrite32 (RPI5D_UART_BASE + UART_IBRD, Divisor);
  MmioWrite32 (RPI5D_UART_BASE + UART_FBRD, 0);
  
  MmioWrite32 (RPI5D_UART_BASE + UART_LCRH, 0x70);
  MmioWrite32 (RPI5D_UART_BASE + UART_CR, 0x301);
  MmioWrite32 (RPI5D_UART_BASE + UART_ICR, 0x7FF);
  
  return RETURN_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval >0               Actual number of bytes written to serial device.
**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  UINTN  Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while (MmioRead32 (RPI5D_UART_BASE + UART_FR) & FR_TXFF);
    MmioWrite32 (RPI5D_UART_BASE + UART_DR, *Buffer);
  }

  return Count;
}

/**
  Read data from serial device and save the data in buffer.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval >0               Actual number of bytes read from serial device.
**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  )
{
  return 0;
}

/**
  Polls serial device to see if there is any data waiting to be read.

  @retval TRUE             Data is waiting to be read from the serial device.
  @retval FALSE            There is no data waiting to be read from the serial device.
**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  return !(MmioRead32 (RPI5D_UART_BASE + UART_FR) & FR_RXFE);
}

/**
  Sets the control bits on a serial device.

  @param Control                Sets the bits of Control that are settable.

  @retval RETURN_SUCCESS        The new control bits were set on the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.
**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32  Control
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Retrieve the status of the control bits on a serial device.

  @param Control                A pointer to return the current control signals from the serial device.

  @retval RETURN_SUCCESS        The control bits were read from the serial device.
  @retval RETURN_UNSUPPORTED    The serial device does not support this operation.
  @retval RETURN_DEVICE_ERROR   The serial device is not functioning correctly.
**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32  *Control
  )
{
  *Control = 0;
  return RETURN_SUCCESS;
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receive time out, parity,
  data bits, and stop bits on a serial device.

  @param BaudRate           The requested baud rate. A BaudRate value of 0 will use the
                            device's default interface speed.
  @param ReceiveFifoDepth   The requested depth of the FIFO on the receive side. A ReceiveFifoDepth
                            value of 0 will use the device's default FIFO depth.
  @param Timeout            The requested time out for a single character in microseconds.
                            This timeout applies to both the transmit and receive side of the
                            interface. A Timeout value of 0 will use the device's default time
                            out value.
  @param Parity             The type of parity to use on this serial device. A Parity value of
                            DefaultParity will use the device's default parity value.
  @param DataBits           The number of data bits to use on the serial device. A DataBits
                            value of 0 will use the device's default data bit setting.
  @param StopBits           The number of stop bits to use on this serial device. A StopBits
                            value of DefaultStopBits will use the device's default number of
                            stop bits.

  @retval RETURN_SUCCESS            The new attributes were set on the serial device.
  @retval RETURN_UNSUPPORTED        The serial device does not support this operation.
  @retval RETURN_INVALID_PARAMETER  One or more of the attributes has an unsupported value.
  @retval RETURN_DEVICE_ERROR       The serial device is not functioning correctly.
**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT UINT32              *Timeout,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  return RETURN_UNSUPPORTED;
}
