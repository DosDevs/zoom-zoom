; ---------------------------------------------------------------
; ------- Copyright 2016 Daniel Valencia, All Rights Reserved. --
; -------                                                      --
; ------- Paging.asm                                           --
; -------                                                      --
; ------- Provides three types of procedures:                  --
; -------   1. Low-level functions to manipulate page tables.  --
; -------   2. High-level functions to set up paging in Start. --
; -------   3. C-style wrappers for type-1 procedures.         --
; ---------------------------------------------------------------


; ----------------------------------------------------------------
; -- This is how paging works in LM64:                          --
; --                                                            --
; -- The 64-bit paging (called IA-32e by Intel) is              --
; -- enabled by having the following bits set to 1:             --
; --    CR0.PE(0) - Protection Enable.                          --
; --    CR0.PG(31) - Paging.                                    --
; --    CR4.PAE(5) - Physical Address Extension.                --
; --    EFER.LME(7) - Long-Mode Enable.                         --
; --                                                            --
; -- The operation is controlled by the following registers:    --
; --      CR2 - Page-Fault Linear Address.                      --
; --      CR3 - Page Directory Base Register.                   --
; --                                                            --
; --                                                            --
; -- The flow is as follows:                                    --
; --                                                            --
; --      1. CR3 indicates where to find the Page Map Level 4,  --
; --         which is nothing more than a table containing      --
; --         addresses of Page Directory Pointer tables.        --
; --
; --      2. Each PDP table contains entries, each of which     --
; --         can point to a Page Directory table, or map        --
; --         a virtual memory page of 1 GiB size.               --
; --                                                            --
; --      3. Each Page Directory itself is a table whose        --
; --         entries can either point to Page Tables, or        --
; --         map 2 MiB virtual memory pages.                    --
; --                                                            --
; --      4. Each one of the Page Table entries contains        --
; --         a mapping to correlate a 4 KiB virtual memory page --
; --         to its location in physical memory.                --
; --                                                            --
; --                                                            --
; -- Virtual memory resolution with 4 KiB pages                 --
; --    When a virtual memory address comes to the CPU for      --
; --    decoding, the PML4 is indexed with bits 39-47, then     --
; --    the PDPT to which that entry is indexed with bits 30-38,--
; --    from which a PD is selected, then bits 21-29 are used   --
; --    to index it to select the PT, which is indexed          --
; --    by bits 12-20 to find the requested page.  This gives   --
; --    us the physical address of this 4 KiB region where we   --
; --    can finally access the data we want by adding the offset--
; --    from bits 0-11.                                         --
; --                                                            --
; -- Virtual memory resolution with 2 MiB pages                 --
; --    It works similarly to resolving using 4 KiB pages,      --
; --    except there is no Page Table in the end, so the nine   --
; --    bits that would be used to index it are instead used    --
; --    as most-significant bits of the (now 21-bits long)      --
; --    offset, and the PD entry points directly to the physical--
; --    addres of the page.                                     --
; --                                                            --
; -- Virtual memory resolution with 1 GiB pages                 --
; --    This mechanism allows to remove yet another layer       --
; --    of indirection, by having the PDPT entry point directy  --
; --    to the physical page.  In this case, the offset is      --
; --    extended to take the 30 bits that would otherwise be    --
; --    used to index the Page Directory and Page Table.        --
; --                                                            --
; --   +---------+---------+---------+---------+------------+   --
; --   | PML4 idx| PDPT idx| PD Index| PT Index|   Offset   |   --
; --   +---------+---------+---------+---------+------------+   --
; --  48        39        30        21        12            0   --
; --                                                            --
; --   +---------+---------+---------+----------------------+   --
; --   | PML4 idx| PDPT idx| PD Index| -----  Offset -----  |   --
; --   +---------+---------+---------+----------------------+   --
; --  48        39        30        21                      0   --
; --                                                            --
; --   +---------+---------+--------------------------------+   --
; --   | PML4 idx| PDPT idx| ---------   Offset   --------- |   --
; --   +---------+---------+--------------------------------+   --
; --  48        39        30                                0   --
; --                                                            --
; --  That said, in order to create a new Page Table entry,     --
; --  we need to ensure the full plumbing exists, so we must    --
; --  make sure we have all of the space needed to allocate     --
; --  these tables.  There are two options: set aside a chunk   --
; --  of physical memory fo all of the tables you will ever     --
; --  need, or allocate them dynamically as they are required.  --
; --                                                            --
; --  For this exercise, we will assume we have at least 2 MiB  --
; --  of physical RAM.  We will create a single 2 MiB page,     --
; --  identity-mapped at the beginning of both addres spaces,   --
; --  with full permissions for read, write and execute.        --
; --  it will be created in PM32 mode, and it will be conserved --
; --  in LM64 throughout the lifetime of the system.  This page --
; --  will be reserved for system-level data structures, and no --
; --  code, --kernel or user-- will be executed from it after   --
; --  the system has bootstrapped.                              --
; --                                                            --
; --  We will assume that 1 GiB is plenty for the boot process  --
; --  and allocate the 4096 bytes for a fully-populated Page    --
; --  Directory, which originally will only contain the basic   --
; --  identity-mapped 2 MiB page at 0.  More entries will be    --
; --  added to it as needed and, should we ever need another    --
; --  Page Directory, it is not terribly difficult to add it    --
; --  at that time.  This will only take care of System pages,  --
; --  meaning those pages nneded by the boot sector, Start,     --
; --  and the Kernel Four (Elfo, Memo, Anjo and IOIO).          --
; ----------------------------------------------------------------


; ----------------------------------------------------------------
; --  Creating a brand-new 4 KiB Page                           --
; --    We have a bitmap of all physical RAM regions, starting  --
; --    at physical address 1 MiB.  The first PML4 has access to--
; --    512 GiB of physical memory, so it shall be enough       --
; --    for most cases.  The full PML4 table requires 4 KiB and --
; --    gives access to 256 TiB of physical memory.             --
; --                                                            --
; ----------------------------------------------------------------

; ---------------------------------------------------------
; -- Functions to manipulate page tables.                --
; ---------------------------------------------------------

;   Functions to create a virtual memory page.
;
;   Input:  RAX = Page virtual address.
;           RDX = Page physical address.
;

Create_4K_Page:
    push rax
    call Get_Tables_For_4K_Page
    pop rax
ret

Get_Tables_For_4K_Page:
    push rax
    push rbx
    push rdx

    mov [Virtual_Address], rax

    ; Get PML4 address
    mov rax, cr3
    and rax, REVERSE_TWELVE_BIT_MASK
    mov [PML4], rax

    ; Resolve addresses of the other tables.
    ; Place them in RBX, RCX, RDX.
    cmp rax, 0
    je Get_Tables_For_4K_Page.No_PML4

    mov rax, [Virtual_Address]
    shr rax, 34 - 4
    and rax, MEM_TABLE_OFFSET_MASK
    add rax, [PML4]
    push rax
    mov rax, [rax]
    mov [PML4_Entry], rax
    mov rbx, rax
    and rbx, PML4_E_ADDRESS_MASK
    mov [PDPT], rbx
    pop rsi
    mov rdi, msgPml4Entry
    mov rdx, PML4_E_ADDRESS_MASK
    call Show_Page_Entry

    ; If PML4 entry is not present, create it.
    mov rax, [PML4_Entry]
    and rax, 1
    jz Get_Tables_For_4K_Page.Create_PML4_Entry

    mov rax, [Virtual_Address]
    shr rax, 30 - 4
    and rax, MEM_TABLE_OFFSET_MASK
    add rax, [PDPT]
    push rax
    mov rax, [rax]
    mov [PDPT_Entry], rax
    mov rbx, rax
    and rbx, PT_E_ADDRESS_MASK
    mov [PD], rbx
    pop rsi
    mov rdi, msgPdptEntry
    mov rdx, PT_E_ADDRESS_MASK
    call Show_Page_Entry

    ; If PDPT entry is not present, create it.
    mov rax, [PDPT_Entry]
    and rax, 1
    jz Get_Tables_For_4K_Page.Create_PDPT_Entry

;   RSI - Page Table Address.
;   RAX - Entry Number.
;   RBX - Entry Variable Pointer.
;   RCX - Next Table Address Pointer.
;   RDI - Page Table Name.
;   RDX - Mask for entries which do not refer to other tables.
    mov rsi, [PD]
    mov rax, [Virtual_Address]
    shr rax, 21
    mov rbx, PT
    mov rcx, 0
    mov rdi, msgPdEntry
    mov rdx, PD_PE_ADDRESS_MASK
    call Get_Page_Details

;    ; If PD entry is not present, create it.
;    mov rax, [PD_Entry]
;    and rax, 1
;    jz Get_Tables_For_4K_Page.Create_PD_Entry

    pop rax
    pop rbx
    pop rdx
ret

; Since this is 64-bit mode, this is impossible.
; We have a bug.  A really, really nasty one.
; So, throw hands in the air and give up.
Get_Tables_For_4K_Page.No_PML4:
    jmp Error

Get_Tables_For_4K_Page.Create_PML4_Entry:
    mov rsi, msgPml4EntryNotPresent
    jmp Error

Get_Tables_For_4K_Page.Create_PDPT_Entry:
    mov rsi, msgPdptEntryNotPresent
    jmp Error

Get_Tables_For_4K_Page.Create_PD_Entry:
    mov rsi, msgPdEntryNotPresent
    jmp Error

Get_Tables_For_4K_Page.Create_PT_Entry:
    mov rsi, msgPtEntryNotPresent
    jmp Error


; Get_Page_Details:
;
;   Looks up a page table entry and provides details if it exists.
;
; Inputs:
;   RSI - Page Table Address.
;   RAX - Entry Number.
;   RBX - Entry Variable Pointer.
;   RCX - Next Table Address Pointer.
;   RDI - Page Table Name.
;   RDX - Mask for entries which do not refer to other tables.
;
; Outputs:
;   RAX - Page Physical Address or reference handle.
;  [RBX]- Entry bits will be stored here.
;  [RCX]- Next table pointer will be stored in this address.
;   CF  - Set to 1 if entry refers to next-level table or 4 KiB page.
;         Cleared to zero if entry refers to a page (4 KiB, 2 MiB or 1 GiB).
;         Undefined for 4KB Pages.
;   ZF  - Page or Page Table not found.
;
Get_Page_Details:
    shl rax, 4
    and rax, MEM_TABLE_OFFSET_MASK
    add rax, rsi
    push rax
    mov rax, [rax]
    mov [tmpEntryAddress], rax
    mov [rbx], rax
    mov rbx, rax
    and rbx, 1
    jz Get_Page_Details.Entry_Not_Present

    mov rbx, rax
    and rbx, 80
    jz Get_Page_Details.Sub_Page_Or_4K

    and rax, PT_E_ADDRESS_MASK

Get_Page_Details.Finish_Up:
    call Show_Page_Entry

    ; If PDPT entry is not present, create it.
    mov rax, [PDPT_Entry]
    and rax, 1
    jz Get_Tables_For_4K_Page.Create_PDPT_Entry

    clz
    clc
ret

Get_Page_Details.Entry_Not_Present:
    pop rax

        push rax
        push rsi
        mov rax, rcx
        mov rsi, msgRAX
        call Show_QWORD
        pop rsi
        pop rax

    mov rdx, PT_E_ADDRESS_MASK
    and rax, rdx
    or rcx, rcx
    jz Get_Page_Details.Entry_Not_Present.End
    mov [rcx], rax

Get_Page_Details.Entry_Not_Present.End:
    stz
ret

Get_Page_Details.Sub_Page_Or_4K:
    pop rsi
    stc
jmp Get_Page_Details.Finish_Up

; [tmpEntryAddress] = Entry Address
; RDI = Entry name address
; RDX = Address Mask
Show_Page_Entry:
    push rdx

    mov rsi, rdi
    call Print
    mov rax, [tmpEntryAddress]
    call Print_QWORD

    call Colon
    call Space
    mov rax, [tmpEntryAddress]
    mov rax, [rax]
    push rax
    call Print_QWORD

    call Space
    mov rsi, msgAddress
    call Print
    pop rax
    pop rdx
    and rax, rdx
    call Print_QWORD

    call New_Line
ret

;   00       00       00       00        00       20       00       00h
;   00000000 00000000 00000000 00000000  00000000 00100000 00000000 00000000
;  +--------+--------+--------+--------++--------+--------+--------+--------+
;  |00000000|00000000|00000000|00000000||00000000|00000000|00000000|00000000|
;  +--------+--------+--------+--------++--------+--------+--------+--------+
; 64       56       48       40       32        24       16        8        0

MEM_TABLE_OFFSET_MASK       EQU 00000000000001ff0h ; 0001 1111 1111 0000
TWELVE_BIT_MASK             EQU 00000000000000fffh ; 0000 1111 1111 1111
REVERSE_TWELVE_BIT_MASK     EQU 0fffffffffffff000h ; 0000 1111 1111 1111
PML4_E_ADDRESS_MASK         EQU 0000ffffffffff000h ; Keep bits 51-12.
PDPT_PE_ADDRESS_MASK        EQU 0000ffffffc000000h ; Keep bits 51-30
PD_PE_ADDRESS_MASK          EQU 0000fffffffe00000h ; Keep bits 51-21.
PT_E_ADDRESS_MASK           EQU 0000ffffffffff000h ; Keep bits 51-12.

msgPml4Entry    db  "PML4 Entry @", 0
msgPdptEntry    db  "PDPT Entry @", 0
msgPdEntry      db  "PD Entry @", 0
msgPtEntry      db  "PT Entry @", 0
msgNextEntry    db  "Next Entry:", 0
msgCR3          db  "CR3: ", 0
msgRAX          db  "RAX: ", 0

tmpEntryAddress dq 0
Virtual_Address dq 0

PML4            dq 0
PDPT            dq 0
PD              dq 0
PT              dq 0

PML4_Entry      dq 0
PDPT_Entry      dq 0
PD_Entry        dq 0
PT_Entry        dq 0

msgPml4EntryNotPresent      db "PML4 Entry not present!", 0
msgPdptEntryNotPresent      db "PDPT Entry not present!", 0
msgPdEntryNotPresent        db "PD Entry not present!", 0
msgPtEntryNotPresent        db "PT Entry not present!", 0

msgAddress      dq "Address:", 0
msWritable      dq "Writable", 0
msgCode         dq "Executable", 0

