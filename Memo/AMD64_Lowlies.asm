[BITS 64]

global Get_Paging_Root
global Set_Paging_Root
global Count_Bits_Set
global Get_Stack_Pointer

Get_Paging_Root:
    mov rax, cr3
ret

Set_Paging_Root:
    mov cr3, rdi
ret

Count_Bits_Set:
    popcnt rax, rdi
ret

Get_Stack_Pointer:
    mov rax, rsp
ret
