#include <os/mm.h>
#include <os/list.h>
#include <pgtable.h>
#include <os/sched.h>

ptr_t memCurr = FREEMEM;

ptr_t allocPage()
{

    // align PAGE_SIZE
    ptr_t ret = ROUND(memCurr, PAGE_SIZE);
    memCurr = ret + PAGE_SIZE;
    return memCurr;
}

void* kmalloc(size_t size)
{
    ptr_t ret = ROUND(memCurr, 4);
    memCurr = ret + size;
    return (void*)ret;
}
uintptr_t shm_page_get(int key)
{
    // TODO(c-core):
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    kmemcpy(dest_pgdir, src_pgdir, PAGE_SIZE);
}

/* allocate physical page for `va`, 
   mapping it into first level page directory `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int mode)
{
    // for debug:
    if (va == 0x11000){
        int debug = 1;
    }
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((vpn2 << (PPN_BITS + PPN_BITS)) | (vpn1 << PPN_BITS)) ^
                    (va >> NORMAL_PAGE_SHIFT);

    PTE *pgdir_2 = (PTE *)pgdir + vpn2;
    PTE *pgdir_1 = NULL;
    PTE *pgdir_0 = NULL;

    if (*pgdir_2 == 0) {
        // alloc a new third-level page directory
        set_pfn(pgdir_2, kva2pa(allocPage()) >> NORMAL_PAGE_SHIFT);
        set_attribute(pgdir_2, (mode << 4 & _PAGE_USER) | _PAGE_PRESENT);
        clear_pgdir(pa2kva(get_pa(*pgdir_2)));
    }
    pgdir_1 = (PTE *)pa2kva(get_pa(*pgdir_2)) + vpn1;
    if (*pgdir_1 == 0) {
        //alloc a new second-level page directory
        set_pfn(pgdir_1, kva2pa(allocPage()) >> NORMAL_PAGE_SHIFT);
        set_attribute(pgdir_1, (mode<<4 & _PAGE_USER) | _PAGE_PRESENT);
        clear_pgdir(pa2kva(get_pa(*pgdir_1)));
    }
    pgdir_0 = (PTE *)pa2kva(get_pa(*pgdir_1)) + vpn0;
    if (*pgdir_0 == 0) {
        ptr_t kva = allocPage();
        set_pfn(pgdir_0, kva2pa(kva) >> NORMAL_PAGE_SHIFT);
        set_attribute(pgdir_0, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | (mode<<4 & _PAGE_USER) | 
                                    _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
        return kva;
    } else {
        return pa2kva(*pgdir_0); //FIXME:??
    }
    

    // return kva;
    // uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + 2 * PPN_BITS)) & VPN_MASK;
    // uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) & VPN_MASK;
    // uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;

    // PTE *first_level_pgdir = (PTE *)pgdir + vpn2;

    // if ((*first_level_pgdir & _PAGE_PRESENT) == 0) {
    //     ptr_t newpage = allocPage();
    //     set_pfn(first_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    //     set_attribute(first_level_pgdir, _PAGE_USER | _PAGE_PRESENT);
    // }

    // PTE *second_level_pgdir = (PTE *)pa2kva(get_pa(*first_level_pgdir)) + vpn1;

    // if ((*second_level_pgdir & _PAGE_PRESENT) == 0) {
    //     ptr_t newpage = allocPage();
    //     set_pfn(second_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    //     set_attribute(second_level_pgdir, _PAGE_USER | _PAGE_PRESENT);
    // } 

    // PTE *last_level_pgdir = (PTE *)pa2kva(get_pa(*second_level_pgdir)) + vpn0;

    // if ((*last_level_pgdir & _PAGE_PRESENT) == 0) {
    //     ptr_t newpage = allocPage();
    //     set_pfn(last_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    //     set_attribute(last_level_pgdir, _PAGE_USER | _PAGE_EXEC | _PAGE_WRITE | _PAGE_READ | _PAGE_PRESENT);
    //     return newpage;
    // } else {
    //     return pa2kva(get_pa(last_level_pgdir));
    // }
}

uintptr_t alloc_page_phys(uintptr_t va, uintptr_t pgdir, uint64_t pa, int mode) {
     va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((vpn2 << (PPN_BITS + PPN_BITS)) | (vpn1 << PPN_BITS)) ^
                    (va >> NORMAL_PAGE_SHIFT);

    PTE *pgdir_2 = (PTE *)pgdir + vpn2;
    PTE *pgdir_1 = NULL;
    PTE *pgdir_0 = NULL;

    if (*pgdir_2 == 0) {
        // alloc a new third-level page directory
        set_pfn(pgdir_2, kva2pa(allocPage()) >> NORMAL_PAGE_SHIFT);
        set_attribute(pgdir_2, (mode << 4 & _PAGE_USER) | _PAGE_PRESENT);
        clear_pgdir(pa2kva(get_pa(*pgdir_2)));
    }
    pgdir_1 = (PTE *)pa2kva(get_pa(*pgdir_2)) + vpn1;
    if (*pgdir_1 == 0) {
        //alloc a new second-level page directory
        set_pfn(pgdir_1, kva2pa(allocPage()) >> NORMAL_PAGE_SHIFT);
        set_attribute(pgdir_1, (mode<<4 & _PAGE_USER) | _PAGE_PRESENT);
        clear_pgdir(pa2kva(get_pa(*pgdir_1)));
    }
    pgdir_0 = (PTE *)pa2kva(get_pa(*pgdir_1)) + vpn0;
    if (*pgdir_0 == 0) {
        ptr_t kva = pa2kva(pa);
        set_pfn(pgdir_0, kva2pa(kva) >> NORMAL_PAGE_SHIFT);
        set_attribute(pgdir_0, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | (mode<<4 & _PAGE_USER) | 
                                    _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
        return kva;
    } else {
        return pa2kva(*pgdir_0); //FIXME:??
    }
}

uintptr_t elf_alloc_page_helper(uintptr_t va, uintptr_t pgdir){
    return alloc_page_helper(va, pgdir, 1);
}


/* Free all pages occupied by one process,
   and add them into freePageList.
    */
extern void freePage(list_head *pagelist) {
    
}

uintptr_t find_pte(uintptr_t va, uintptr_t pgdir)
{
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((vpn2 << (PPN_BITS + PPN_BITS)) | (vpn1 << PPN_BITS)) ^
                    (va >> NORMAL_PAGE_SHIFT);
    
    PTE *pgdir_2 = (PTE *)pgdir + vpn2;
    if (((*pgdir_2) & _PAGE_PRESENT) == 0) {
        return 0;
    }

    PTE *pgdir_1 = (PTE *)get_va(*pgdir_2) + vpn1;
    if (((*pgdir_1) & _PAGE_PRESENT) == 0) {
        return 0;
    }

    PTE *pgdir_0 = (PTE *)get_va(*pgdir_1) + vpn0;
    if (((*pgdir_0) & _PAGE_PRESENT) == 0) {
        return 0;
    }
    return pgdir_0;
}