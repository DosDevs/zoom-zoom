; ---------------------------------------------------------------
; ------- Copyright 2015 Daniel Valencia, All Rights Reserved. --
; -------                                                      --
; ------- Elf64.asm                                            --
; -------                                                      --
; ------- Interprets and executes simple ELF programs.         --
; -------                                                      --
; ------- RSI - Pointer to start of buffer.                    --
; ------- RCX - Size of buffer in bytes.                       --
; ---------------------------------------------------------------

[BITS 64]
Elf_64_Entry:
        ;
        ; Find program Table.
        ;
    lodsd
    cmp eax, ELF_64_MAGIC
    jne Elf_64_Not_Elf

    lodsd  ; Assume 64-bit Little-Endian; ignore.
    lodsq  ; Ignore Padding.
    lodsq  ; Ignore Type, Machine, Program_Version.

    lodsq  ; Program entry point.
    mov [Elf_64_Program_Entry_Point], rax

    lodsq  ; Program header offset.
    mov [Elf_64_Program_Table], rax

    lodsq  ; Section header offset.
    mov [Elf_64_Section_Table], rax
    
    lodsd  ; Ignore flags
    lodsw  ; Elf header size in bytes.
    mov [Elf_64_Header_Size], ax

    lodsw  ; Program header entry size.
    mov [Elf_64_Program_Header_Entry_Size], ax

    lodsw  ; Program header entry count.
    mov [Elf_64_Program_Header_Entry_Count], ax

    lodsw  ; Section header entry size.
    mov [Elf_64_Section_Header_Entry_Size], ax

    lodsw  ; Section header entry count.
    mov [Elf_64_Section_Header_Entry_Count], ax

    ;
    ; Parse the Program Table.
    ;
    mov rsi, [Elf_64_Program_Table]
    add rsi, EL_ADDRESS

    lodsd
    mov [Elf_64_Segment_Type], eax

    lodsd
    mov [Elf_64_Segment_Flags], eax

    lodsq
    mov [Elf_64_Segment_Offset], rax

    lodsq
    mov [Elf_64_Segment_Address], rax

    lodsq  ; Ignore physical address.
    lodsq
    mov [Elf_64_Segment_File_Size], rax

    lodsq
    mov [Elf_64_Segment_Memory_Size], rax

    lodsq
    mov [Elf_64_Segment_Alignment], rax

    mov rax, [Elf_64_Program_Entry_Point]
ret

Elf_64_Not_Elf:
    stc
    mov rax, 0ffffffffffffffffh
ret


ELF_64_MAGIC        EQU 0464c457fh

Elf_64_Program_Entry_Point          dq 0
Elf_64_Program_Table                dq 0
Elf_64_Section_Table                dq 0
Elf_64_Header_Size                  dw 0
Elf_64_Program_Header_Entry_Size    dw 0
Elf_64_Program_Header_Entry_Count   dw 0
Elf_64_Section_Header_Entry_Size    dw 0
Elf_64_Section_Header_Entry_Count   dw 0

Elf_64_Segment_Type         dd 0
Elf_64_Segment_Flags        dd 0
Elf_64_Segment_Offset       dq 0
Elf_64_Segment_Address      dq 0
Elf_64_Segment_File_Size    dq 0
Elf_64_Segment_Memory_Size  dq 0
Elf_64_Segment_Alignment    dq 0

msgProgramEntry db "Program entry point: ", 0
msgProgramTableOffset db "Program table offset: ", 0
msgSectionTableOffset db "Section table offset: ", 0
msgSegmentOffset db "Segment offset: ", 0
msgSegmentAddress db "Segment address: ", 0
msgSegmentFileSize db "Segment size in file: ", 0
msgSegmentMemory db "Segment size in memory: ", 0

