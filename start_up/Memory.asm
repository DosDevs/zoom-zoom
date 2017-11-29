



[BITS 64]

; Creates a page table.
;   RAX=Physical address.
;   RBX=Virtual address.
;   RCX=Page size:
;       0 - 4KiB
;       1 - 2MiB
;       3 - 1GiB
Create_Page_Table_32:
    mov rdi, cr3


Create_Page_Table_Entry:

