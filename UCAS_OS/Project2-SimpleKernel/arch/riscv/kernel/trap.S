#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* save exception_handler_entry into STVEC
   * enable global exceptions */
  la s0, exception_handler_entry
  csrw CSR_STVEC, s0
ENDPROC(setup_exception)
