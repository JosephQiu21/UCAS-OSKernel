#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* save exception_handler_entry into STVEC
   * enable global exceptions */
  la t0, exception_handler_entry
  csrw CSR_STVEC, t0
  li t0, SR_SIE
  csrw sstatus, t0
  li s0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrw sie, s0
  jr ra
ENDPROC(setup_exception)
