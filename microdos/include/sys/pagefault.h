/*
Pagefault error code bitfield
*/
#define PAGEFAULT_ERR_PRESENT 1<< 0 // 	When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
#define PAGEFAULT_ERR_WRITE   1<< 1 //When set, the page fault was caused by a write access. When not set, it was caused by a read access.
#define PAGEFAULT_ERR_USER    1<< 2 // When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
#define PAGEFAULT_ERR_RESERVEDWRITE 1<< 3 // 	When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
#define PAGEFAULT_ERR_INSFETCH 1<< 4 //When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.
#define PAGEFAULT_ERR_PKEY 1<< 5    // 	When set, the page fault was caused by a protection-key violation. The PKRU register (for user-mode accesses) or PKRS MSR (for supervisor-mode accesses) specifies the protection key rights.
#define PAGEFAULT_ERR_SS 1<< 6      // When set, the page fault was caused by a shadow stack access.
#define PAGEFAULT_ERR_SGX 1<< 15    //When set, the fault was due to an SGX violaton. The fault is unrelated to ordinary paging.
#define PAGEFAULT_ERR_PRESENT 1<< 0