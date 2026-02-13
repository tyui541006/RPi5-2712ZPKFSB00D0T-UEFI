#include <AsmMacroIoLibV8.h>

.section .text, "ax"

ASM_FUNC (MmioRead32)
  ldr   w0, [x0]
  ret

ASM_FUNC (MmioWrite32)
  str   w1, [x0]
  ret
