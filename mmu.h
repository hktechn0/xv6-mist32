// This file contains definitions for the 
// mist32 memory management unit (MMU).

#define PSR_MMUMOD_DIRECT 0x0
#define PSR_MMUMOD_L1 0x1
#define PSR_MMUMOD_L2 0x2
#define PSR_IM 0x4
#define PSR_CMOD_KERNEL 0x00
#define PSR_CMOD_USER 0x60
#define PSR_MMUPS_4KB 0x80

// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/ 

// page directory index
#define PDX(va)         (((uint)(va) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(va)         (((uint)(va) >> PTXSHIFT) & 0x3FF)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPDENTRIES      1024    // # directory entries per page directory
#define NPTENTRIES      1024    // # PTEs per page table
#define PGSIZE          4096    // bytes mapped by a page

#define PGSHIFT         12      // log2(PGSIZE)
#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        22      // offset of PDX in a linear address

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page table/directory entry flags.
#define PTE_V           0x001   // Valid
#define PTE_R           0x002   // Read
#define PTE_D           0x004   // Dirty
#define PTE_EX          0x008   // Executable
#define PTE_PP          0x030   // Privilege
#define PTE_PP_RWXX     0x000   //   K: RW U: XX
#define PTE_PP_RDXX     0x010   //   K: RD U: XX
#define PTE_PP_RWRD     0x020   //   K: RW U: RD
#define PTE_PP_RWRW     0x030   //   K: RW U: RW
#define PTE_CD          0x040   // Cache-Disable
#define PTE_G           0x080   // Global
#define PTE_PE          0x100   // Extended Page Size

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint)(pte) &  0xFFF)

#ifndef __ASSEMBLER__
typedef uint pte_t;

#endif
