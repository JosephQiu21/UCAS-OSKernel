#include <os/mm.h>
#include <os/list.h>
#include <pgtable.h>
#include <os/sched.h>

ptr_t memCurr = FREEMEM;

static LIST_HEAD(freePageList);

ptr_t allocPage()
{
    // if (!is_list_empty(&freePageList)) {
    //     page_t *page = container_of(dequeue(&freePageList), page_t, list);
    //     return page -> kva;
    // }
    // // align PAGE_SIZE
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
    kmemcpy(dest_pgdir, src_pgdir, NORMAL_PAGE_SIZE);
}

/* allocate physical page for `va`, 
   mapping it into first level page directory `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, mode_t mode)
{
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0 << PPN_BITS);
    uint64_t vpn1 = (va >> NORMAL_PAGE_SHIFT + PPN_BITS) & ~(~0 << PPN_BITS);
    uint64_t vpn2 = (va >> NORMAL_PAGE_SHIFT + 2 * PPN_BITS) & ~(~0 << PPN_BITS);
    PTE *first_level_pgdir = (PTE *)pgdir + vpn2;
    if ((*first_level_pgdir & _PAGE_PRESENT) == 0)
    {
        ptr_t newpage = allocPage();
        set_pfn(first_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        set_attribute(first_level_pgdir, (mode ? _PAGE_USER : 0) | _PAGE_PRESENT);
    }
    PTE *second_level_pgdir = (PTE *)pa2kva(get_pa(*first_level_pgdir)) + vpn1;
    if ((*second_level_pgdir & _PAGE_PRESENT) == 0)
    {
        ptr_t newpage = allocPage();
        set_pfn(second_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        set_attribute(second_level_pgdir, (mode ? _PAGE_USER : 0) | _PAGE_PRESENT);
    }
    PTE *last_level_pgdir = (PTE *)pa2kva(get_pa(*second_level_pgdir)) + vpn0;
    if ((*last_level_pgdir & _PAGE_PRESENT) == 0)
    {
        ptr_t newpage = allocPage();
        set_pfn(last_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        set_attribute(last_level_pgdir, _PAGE_DIRTY | _PAGE_ACCESSED | (mode ? _PAGE_USER : 0) | _PAGE_EXEC | _PAGE_WRITE | _PAGE_READ | _PAGE_PRESENT);
        return newpage;
    }
    return 0;
    // uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + 2 * PPN_BITS)) & VPN_MASK;
    // uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) & VPN_MASK;
    // uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;

    // PTE *first_level_pgdir = (PTE *)pgdir + vpn2;

    // if ((*first_level_pgdir & _PAGE_PRESENT) == 0) {
    //     ptr_t newpage = allocPage();
    //     set_pfn(first_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    //     set_attribute(first_level_pgdir, (mode == USER_MODE) ? _PAGE_USER : 0 | _PAGE_PRESENT);
    // }

    // PTE *second_level_pgdir = (PTE *)pa2kva(get_pa(*first_level_pgdir)) + vpn1;

    // if ((*second_level_pgdir & _PAGE_PRESENT) == 0) {
    //     ptr_t newpage = allocPage();
    //     set_pfn(second_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    //     set_attribute(second_level_pgdir, (mode == USER_MODE) ? _PAGE_USER : 0 | _PAGE_PRESENT);
    // } 

    // PTE *last_level_pgdir = (PTE *)pa2kva(get_pa(*second_level_pgdir)) + vpn0;

    // if ((*last_level_pgdir & _PAGE_PRESENT) == 0) {
    //     ptr_t newpage = allocPage();
    //     set_pfn(last_level_pgdir, kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
    //     set_attribute(last_level_pgdir, (mode == USER_MODE) ? _PAGE_USER : 0 | _PAGE_EXEC | _PAGE_WRITE | _PAGE_READ | _PAGE_PRESENT);
    //     return newpage;
    // }
    // return 0;
}

uintptr_t elf_alloc_page_helper(uintptr_t va, uintptr_t pgdir){
    return alloc_page_helper(va, pgdir, USER_MODE);
}

/* Check whether va has been mapped,
   mapped -> return kva of page,
   unmapped -> return 0.
   */
uintptr_t check_page_helper(uintptr_t va, uintptr_t pgdir) {
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + 2 * PPN_BITS)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;

    PTE *first_level_pgdir = (PTE *)pgdir + vpn2;

    if ((*first_level_pgdir & _PAGE_PRESENT) == 0) {
        return 0;
    }

    PTE *second_level_pgdir = (PTE *)pa2kva(get_pa(*first_level_pgdir)) + vpn1;

    if ((*second_level_pgdir & _PAGE_PRESENT) == 0) {
        return 0;
    } 

    PTE *last_level_pgdir = (PTE *)pa2kva(get_pa(*second_level_pgdir)) + vpn0;

    if ((*last_level_pgdir & _PAGE_PRESENT) == 0) {
        return 0;
    }
    return pa2kva(get_pa(last_level_pgdir));
}

/* Free all pages occupied by one process,
   and add them into freePageList.
    */
extern void freePage(list_head *pagelist) {
    while (!is_list_empty(pagelist)) {
        page_t *pcb_page = container_of(dequeue(pagelist), page_t, list);
        enqueue(&freePageList, &(pcb_page -> list));
    }
}