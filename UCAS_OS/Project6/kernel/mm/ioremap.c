#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    uintptr_t pgdir = pa2kva(PGDIR_PA);
    uintptr_t va = io_base;
    uint64_t pa = phys_addr;
    int mode = 0;

    int num_page = size / PAGE_SIZE;

    while ((--num_page) >= 0) {
        alloc_page_phys(io_base, pgdir, pa, mode);
        io_base += PAGE_SIZE;
        pa += PAGE_SIZE;
    }
    local_flush_tlb_all();
    return (void *)va;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
