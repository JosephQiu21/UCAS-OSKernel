#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* save exception_handler_entry into STVEC
   * enable global exceptions */

  la t0, exception_handler_entry
  csrw CSR_STVEC, t0
  li t0, SIE_STIE
  csrw CSR_SIE  , t0
  csrsi CSR_SSTATUS, SR_SIE
  jr ra
ENDPROC(setup_exception)
