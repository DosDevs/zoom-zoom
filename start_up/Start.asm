; ---------------------------------------------------------------
; ------- Copyright 2015 Daniel Valencia, All Rights Reserved. --
; -------                                                      --
; ------- Start.asm                                            --
; -------                                                      --
; ------- Loads managers and prepares the environment          --
; ------- for them to run.  Provides services to log           --
; ------- to screen and to switch between processor            --
; ------- operating modes.                                     --
; ---------------------------------------------------------------


[BITS 16]
[ORG 6000h]

jmp Start

%include "Text_Mode.asm"
%include "Elf_64.asm"
%include "Memory.asm"


[BITS 16]


; ---------------------------------------------------------
; ------- Start:                                         --
; -------                                                --
; -------      Entry Point.                              --
; ---------------------------------------------------------

Start:
    pop dword [fnStart]
    pop dword [fnGetFile]

    mov sp, 0ffffh

    call Clear_Memory

    mov esi, Elfo
    mov bx, EL_ADDRESS / 10h
    call [fnGetFile]
    mov [ElfoSize], cx

    mov esi, Memo
    mov bx, MM_ADDRESS / 10h
    call [fnGetFile]
    mov [MemoSize], cx

    call Init_Video

    call Get_Memory_Map
;    call VESA_Init
;    call PCI_Init

    ; Mark the first 16 bits of the memory bitmap as taken.
    mov word [OriginalMemoryBitmap], 0xffff

    jmp Start_PM
hlt


; ----------------------------------------------------------
; ------- Clear_Memory:                                   --
; -------                                                 --
; -------      Clears memory between 10000h and 80000h,   --
; ------- which corresponds to 1000:0000 thru 8000:0000.  --
; ----------------------------------------------------------
Clear_Memory:
    mov eax, 1000h
    mov ebx, eax
    xor esi, esi
    mov edx, 0f000h
Clear_Memory.Loop:
    mov ecx, ebx
    mov es, ax
    xor edi, edi
    xchg eax, esi
    rep stosb
    mov ax, es
    add ax, bx
    cmp ax, dx
    jb Clear_Memory.Loop
ret


;; ---------------------------------------------------------
;; ------- Gfx_Mode                                       --
;; ---------------------------------------------------------
;
;Gfx_Mode:
;       ;
;       ; D0-D8 = Mode Number.  D8 indicates whether this
;       ;           is a VESA-defined VBE Mode.
;       ;
;       ; D9-D12 = Reserved MBZ.
;       ;
;       ; D11 = Refresh rate control selet. ?????
;       ;
;       ; D12-D13 = Reserved MBZ.
;       ;
;       ; D14 = Linear/flat frame buffer select.
;       ;
;       ; D15 = Preserve display memory.
;       ;
;    mov edx, 11bh
;
;    
;
;    ret


; ---------------------------------------------------------
; ------- VESA_Init:                                     --
; ---------------------------------------------------------

VESA_Init:
    call VESA_Init.FindPmInfoBlock
    call VESA_Init.GetVbeControllerInfo
    ret

VESA_Init.FindPmInfoBlock:
       ; Scan the region 0000-8000 looking for
       ; the VESA protected mode info block.
    mov ebx, 4  ; Amount of bytes to compare at once.

		; Put "PMID" in little endian into eax.
    mov eax, 4449h
    shl eax, 16
    or eax, 4d50h
    mov edx, 8000h

      ; Start by looking at 32-bit aligned addresses for speed.
    xor esi, esi
VESA_Init.Loop:
    cmp dword [esi], eax
    je VESA_Init.PM_Info_Block_Found
    add esi, ebx
    cmp esi, edx
    jl VESA_Init.Loop  ; Inner loop.
    inc edx
    mov esi, edx
    and esi, 0003h  ; Keep trying misaligned addresses.
    jnz VESA_Init.Loop  ; Outer loop.

VESA_Init.PM_Info_Block_Not_Found:
    mov esi, msgVesaPmibNotFound
    call Print_16
    call crlf16
    ret

VESA_Init.PM_Info_Block_Found:
    mov dword [PMInfoBlock], esi
    mov esi, msgFoundVesaPmib
    call Print_16
    mov eax, dword [PMInfoBlock]
    call Print_DWORD_16
    call crlf16
    ret


VESA_Init.GetVbeControllerInfo:
    mov ax, 4f00h
    push cs
    pop es
    lea edi, [VbeInfoBlock]
    int 10h
    mov word [GetVbeControllerInfoResult], ax
    ret


; ---------------------------------------------------------
; ------- PCI_Init:                                     --
; ---------------------------------------------------------

PCI_Init:
    mov dx, 0cf8h
    in eax, dx
    mov dword [PCI_Config_Address], eax
    mov dx, 0cfch
    in eax, dx
    mov dword [PCI_Config_Data], eax
    ret


; ---------------------------------------------------------
; ------- Get_Memory_Map:                                --
; ---------------------------------------------------------

Get_Memory_Map:
    lea esi, [msgGettingMemoryMap]
    call Print_16
    xor ebx, ebx                      ; Start at offset: 0
    mov es, bx                        ; Destination:
    mov edi, MMAP_ADDRESS             ; MMAP_ADDRESS (flat)

Get_Memory_Map.Loop:
    mov eax, 0e820h                   ; Service
    mov edx, 534d4150h                ; Function: 'SMAP'
    mov ecx, 20                       ; Dest. buffer size
    int 15h

    add edi, 20
    inc byte [RAM_Table_Entries]      ; +1 entries

    cmp bx, 0                         ; Check if done...
    jne Get_Memory_Map.Loop
    call crlf16
    ret



;; ---------------------------------------------------------
;; ------- Show_VESA_Info:                                --
;; ---------------------------------------------------------
;
;Show_VESA_Info:
;    call Show_VESA_Info_Block
;    call Show_VESA_VBE_Controller_Info
;    ret
;
;Show_VESA_Info_Block:
;    call crlf16
;
;    lea esi, [PMInfoBlock]
;    test esi, esi
;    jz Show_VESA_Info_Block.NoInfoBlock
;
;    mov eax, esi
;    lea esi, [msgFoundVesaPmib]
;    call Show_DWORD
;
;    mov ebp, [PMInfoBlock]
;
;    mov ax, word [ebp + PMInfoBlock.Signature]
;    mov esi, msgPMInfoBlockSignature
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.EntryPoint]
;    mov esi, msgPMInfoBlockEntryPoint
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.PMInitialize]
;    mov esi, msgPMInfoBlockPMInitialize
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.BiosDataSel]
;    mov esi, msgPMInfoBlockBiosDataSel
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.A0000Sel]
;    mov esi, msgPMInfoBlockA0000Sel
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.B0000Sel]
;    mov esi, msgPMInfoBlockB0000Sel
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.B8000Sel]
;    mov esi, msgPMInfoBlockB8000Sel
;    call Show_WORD
;
;    mov ax, word [ebp + PMInfoBlock.CodeSegSel]
;    mov esi, msgPMInfoBlockCodeSegSel
;    call Show_WORD
;
;    mov al, byte [ebp + PMInfoBlock.InProtectMode]
;    mov esi, msgPMInfoBlockInProtectMode
;    call Show_BYTE
;
;    mov al, byte [ebp + PMInfoBlock.Checksum]
;    mov esi, msgPMInfoBlockChecksum
;    call Show_BYTE
;
;    ret
;
;Show_VESA_Info_Block.NoInfoBlock:
;    lea esi, [msgVesaPmibNotFound]
;    call puts
;    ret
;
;
;Show_VESA_VBE_Controller_Info:
;    call crlf16
;    lea esi, [msgGetVbeControllerInfoResult]
;    mov ax, [GetVbeControllerInfoResult]
;    call Show_WORD
;
;    mov esi, [msgVbeInfoBlock]
;    call puts
;
;    lea esi, [msgVbeInfoBlockVbeSignature]
;    mov eax, VbeInfoBlock.VbeSignature
;    mov ecx, 4
;    call Show_String_N
;
;    lea esi, [msgVbeInfoBlockVbeVersion]
;    mov ax, [VbeInfoBlock.VbeVersion]
;    call Show_WORD
;
;    lea esi, [msgVbeInfoBlockOemStringPtr]
;    mov eax, [VbeInfoBlock.OemStringPtr]
;    call FlattenVbeFarPtr
;    call Show_String
;
;    lea esi, [msgVbeInfoBlockCapabilities]
;    mov al, [VbeInfoBlock.Capabilities]
;    call Show_BYTE
;
;    lea esi, [msgVbeInfoBlockVideoModePtr]
;    mov eax, [VbeInfoBlock.VideoModePtr]
;    call Show_DWORD
;
;    lea esi, [msgVbeInfoBlockTotalMemory]
;    mov ax, [VbeInfoBlock.TotalMemory]
;    call Show_WORD
;
;    lea esi, [msgVbeInfoBlockOemSoftwareRev]
;    mov ax, [VbeInfoBlock.OemSoftwareRev]
;    call Show_WORD
;
;    lea esi, [msgVbeInfoBlockOemVendorNamePtr]
;    mov eax, [VbeInfoBlock.OemVendorNamePtr]
;    call FlattenVbeFarPtr
;    call Show_String
;
;    lea esi, [msgVbeInfoBlockOemProductNamePtr]
;    mov eax, [VbeInfoBlock.OemProductNamePtr]
;    call FlattenVbeFarPtr
;    call Show_String
;
;    lea esi, [msgVbeInfoBlockOemProductRevPtr]
;    mov eax, [VbeInfoBlock.OemProductRevPtr]
;    call FlattenVbeFarPtr
;    call Show_String
;
;Show_VESA_VBE_Controller_Info.ShowModes:
;    call crlf16
;    mov esi, VbeInfoBlock
;    mov ecx, 80h
;    call Dump
;    mov eax, [VbeInfoBlock.VideoModePtr]
;    call FlattenVbeFarPtr
;    mov esi, eax
;    mov ecx, 80h
;    call Dump
;    ret
;
;Show_VESA_VBE_Controller_Info.ShowModes.Loop:
;    lodsw
;    cmp ax, 0ffffh
;    je Show_VESA_VBE_Controller_Info.ShowModes.end
;    call Print_WORD
;    call space
;    loop Show_VESA_VBE_Controller_Info.ShowModes.Loop
;
;Show_VESA_VBE_Controller_Info.ShowModes.end:
;    call crlf16
;    ret
;
;FlattenVbeFarPtr:
;    push ebx
;    mov ebx, eax
;    movzx eax, ax
;    shr ebx, 16
;    shl ebx, 4
;    add eax, ebx
;    pop ebx
;    ret
;
;
;; ---------------------------------------------------------
;; ------- Show_PCI_Info:                                 --
;; ---------------------------------------------------------
;
;Show_PCI_Info:
;    mov esi, msgPciConfigAddress
;    mov eax, [PCI_Config_Address]
;    call Show_DWORD
;
;    mov esi, msgPciConfigData
;    mov eax, [PCI_Config_Data]
;    call Show_DWORD
;    ret
;
;
;; ---------------------------------------------------------
;; ------- Show_Apic_Info:                                --
;; ---------------------------------------------------------
;
;Show_Apic_Info:
;    call crlf16
;
;    mov esi, msgApicId
;    call Print_16
;    mov eax, dword [APIC_ID]
;    call Print_DWORD
;    call crlf16
;    mov esi, msgApicVersion
;    call Print_16
;    mov eax, dword [APIC_VERSION]
;    call Print_DWORD
;    call crlf16
;    ret
;    
;
;; ---------------------------------------------------------
;; ------- Set_Interrupt_Table:                           --
;; ---------------------------------------------------------
;
;
;Set_Interrupt_Table:
;
;
;
;    
;
;
;
;
;
; ---------------------------------------------------------
; ------- Start_PM:                                      --
; -------   Puts CPU in the 32-bit Protected Mode.       --
; ---------------------------------------------------------

Start_PM:
    cli

    lgdt [GDTR]

           ; 31 - Paging: Disabled.
           ; 30 - Cache Disable: Disabled.
           ; 29 - Not-Write-Through: Disabled.
           ; 18 - Alignment Mask: Disabled.
           ; 16 - Write Protect: Disabled.
           ;  5 - Numeric Error: Disabled.
           ;  4 - Extension Type: Disabled.
           ;  3 - Task Switched: Disabled.
           ;  2 - Emulation: Disabled.
           ;  1 - Monitor Co-processor: Disabled.
           ;  0 - Protected Mode Enable: Enabled.
;    mov eax, 60000009h       ; Enable PM, Disable Caches
;    mov cr0, eax
mov eax, cr0
or al, 1
mov cr0, eax

    jmp PM_CODE_SEGMENT:PM_Start



; ---------------------------------------------------------
; ------- PM_Start:                                      --
; -------   Routine to start right after switching into  --
; -------   32-bit Protected Mode.  Its duty: to set up  --
; -------   and start the AMD 64-bit Long Mode.          --
; ---------------------------------------------------------
align 16, db 0
[BITS 32]

PM_Start:
    mov ax, DATA_SEGMENT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ebx, 0b809ch
    mov dword [ebx], 017321733h

    xor eax, eax
    mov edi, 0100000h
    mov ecx, 40000h   ; 100000h / 4
    rep stosd

         ;
         ; Enable the 64-bit page translation table entries
         ;
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

         ;
         ; Initialise the Page Tables
         ;
    mov edi, INITIAL_PML4
    mov cr3, edi                  ; Store PML4's address in CR3

    mov edi, INITIAL_PD
    mov eax, INITIAL_PDE
    call Create_Page_Table

    mov edi, INITIAL_PDPT
    mov eax, INITIAL_PDPTE
    call Create_Page_Table

    mov edi, INITIAL_PML4
    mov eax, INITIAL_PML4E
    call Create_Page_Table

         ;
         ; Enable Long Mode
    mov ecx, 0c0000080h
    rdmsr
    bts eax, 8
    wrmsr

         ;
         ; Enable Paging so Long Mode kicks in
         ;
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

         ;
         ; Jump far to LM_Start
         ;
    db 0eah
    dd Long_Mode_Start
    dw LM_CODE_SEGMENT

Create_Page_Table:
    push edi

    stosd

    ; Put physical page number in EAX
    pop eax
    shr eax, 12
    bts dword [OriginalMemoryBitmap], eax

    xor eax, eax
    mov ecx, 0fdh
    rep stosd
ret


align 16, db 0
[BITS 64]

Long_Mode_Start:
    mov rsp, 10000h

        ;
        ; Color the screen.
        ;
    mov rdi, 0b8001h
    mov al, 2ah
    mov rcx, 2000
Color_The_Screen_Loop:
    stosb
    inc rdi
    loop Color_The_Screen_Loop

    mov rbx, 0b809ch
    mov rax, 2a342a36h
    mov dword [rbx], eax

        ;
        ; Initialize the TSS IO Map.
        ;
    movzx rdi, word [TSS_IO_Map_Address]
    mov rcx, 25
    xor eax, eax
    rep stosd

        ;
        ; Load Task Register (TR).
        ;
    mov ax, TS_SEGMENT
    ltr ax

        ;
        ; Print the "Starting" message.
        ;
    mov rsi, msgStarting
    call Print
    call New_Line

        ;
        ; Export the publics.
        ;
    mov rdi, [Publics]
    mov rax, OriginalMemoryBitmap   ; #0
    stosq
    mov rax, kprint                 ; #1
    stosq
    mov rax, kputchar               ; #2
    stosq
    mov rax, knewline               ; #3
    stosq
    mov rax, ksetcursor             ; #4
    stosq
    mov rax, kgetcursor             ; #5
    stosq

;    call Show_Memory_Map

        ;
        ; Initialize the ELF interpreter.
        ;
    mov rsi, EL_ADDRESS
    mov rcx, [ElfoSize]
    call Elf_64_Entry
    jc Error
    mov [ElfoEntryPoint], rax

        ; Arguments in System V AMD64 ABI's C calling convention:
        ;      RDI, RSI, RDX, RCX, R8, R9.
        ;
        ; Return value:
        ;      RDX:RAX
        ;
        ; #define ACTUATOR_ARGUMENTS
        ;             uint64_t options,
        ;             uint64_t arg_1, uint64_t arg_2,
        ;             uint64_t arg_3, uint64_t arg_4

        ;
        ; Bootstrap the ELF interpreter and the memory manager.
        ;
    mov rdi, 1                      ; options = ELFO_EXECUTE
    mov rsi, [Elfo_Execute_Options] ; Memo bootstrap options
    mov rdx, EL_ADDRESS
    mov rcx, MM_ADDRESS
    mov r8, MMAP_ADDRESS
    movzx r9, byte [RAM_Table_Entries]
    call [ElfoEntryPoint]
    mov [MemoEntryPoint], rax

Halt:
    hlt
jmp Halt

Error:
    call Print
    call New_Line
	mov rsi, msgError
	call Print
	jmp Halt

; ---------------------------------------------------------
; ------- Show_Memory_Map:                               --
; ---------------------------------------------------------

Show_Memory_Map:
    mov esi, msgNumEntries
    call Print
    movzx rax, byte [RAM_Table_Entries]
    push rax
    call Print_DWORD
    call New_Line

    pop rcx                            ; Number of entries
    mov esi, MMAP_ADDRESS

Show_Memory_Map.Loop:
    push rcx

    ; Print base address
    Call Colon
    lodsq
    call Print_QWORD
    ; Print length in bytes
    call Colon
    lodsq
    call Print_QWORD

    ; Print type
    call Colon
    lodsd
    call Print_DWORD
    call Colon
    call New_Line

    ; Retrieve ecx, and loop back.
    pop rcx
    loop Show_Memory_Map.Loop
ret




; ---------------------------------------------------------
; -------                  D a t o s                     --
; ---------------------------------------------------------

INITIAL_PML4        EQU 100000h
INITIAL_PDPT        EQU 101000h
INITIAL_PD          EQU 102000h

INITIAL_PML4E       EQU INITIAL_PDPT + 03bh
INITIAL_PDPTE       EQU INITIAL_PD + 03bh
INITIAL_PDE         EQU 0fbh

NULL_SEGMENT        EQU 0000h
PM_CODE_SEGMENT     EQU 0008h
LM_CODE_SEGMENT     EQU 0010h
DATA_SEGMENT        EQU 0018h
TS_SEGMENT          EQU 0020h
INVALID_SEGMENT_1   EQU 0028h

MM_ADDRESS          EQU 020000h
EL_ADDRESS          EQU 030000h

; Memory Map Constants
MMAP_ADDRESS        EQU 500h
MMAP_LENGTH         EQU 100h
MMAP_AVAILABLE      EQU 1
MMAP_RESERVED       EQU 2
MMAP_ACPI_RECLAIM   EQU 3
MMAP_ACPI_NVS       EQU 4

; Addresses of APIC Memory-Mapped Registers
APIC_ID             EQU 0xfee00020
APIC_VERSION        EQU 0xfee00030

MEMO_SS_BEGIN       EQU 8000h
MEMO_SS_END         EQU 0c000h
MEMO_SS_SIZE        EQU (MEMO_SS_END - MEMO_SS_BEGIN)
MEMO_SS_KILOS       EQU (MEMO_SS_SIZE >> 10)
MEMO_BS_SS_FIELD    EQU ((MEMO_SS_KILOS << 24) | MEMO_SS_BEGIN)

PDE_ADDRESS         EQU 1008h           ; Address of PDE

; Hacking in a 64-bit word in two 32-bit words.  Little-endian.
Elfo_Execute_Options    dq ((MEMO_BS_SS_FIELD << 32) | 0ffffh)

RAM_Size            dq 0           ; Size of Physical Memory (in Bytes)

align 16, db 0
GDTR          dw 2fh
              dd GDT

align 16, db 0
GDT             dd 0, 0  ; dw 0
                dd 0000ffffh, 00cf9f00h
                dd 0000ffffh, 00af9f00h
                dd 0000ffffh, 00cf9300h
TSS_Descriptor  dw 103,TSS, 8900h,0040h
                dq 0000000000000000h

align 16, db 0
TSS               times 100 db   0
TSS_Reserved                dw   0
TSS_IO_Map_Address          dw 800h


align 16, db 0
PMInfoBlock                 dd 00h
PMInfoBlock.Signature       EQU 04h
PMInfoBlock.EntryPoint      EQU 06h
PMInfoBlock.PMInitialize    EQU 08h
PMInfoBlock.BiosDataSel     EQU 0ah
PMInfoBlock.A0000Sel        EQU 0ch
PMInfoBlock.B0000Sel        EQU 0eh
PMInfoBlock.B8000Sel        EQU 10h
PMInfoBlock.CodeSegSel      EQU 12h
PMInfoBlock.InProtectMode   EQU 14h
PMInfoBlock.Checksum        EQU 15h
PMInfoBlock.Size            EQU 16h

Publics dq 0e00h

PCI_Config_Address  dd 0
PCI_Config_Data     dd 0

msgPciConfigAddress db "PCI_CONFIG_ADDRESS: ", 0
msgPciConfigData db "PCI_CONFIG_DATA: ", 0


RAM_Table_Entries   db 0

msgDot              db 2eh, 0
msgStarting         db "Starting.", 0
msgGettingMemoryMap db "Asking BIOS for memory map.", 0
msgNumEntries       db "Number of Memory Map Entries: ", 0
msgBaseAddress      db "     Base Address = ", 0
msgLength           db "     Length = ", 0
msgTypeOfAddress    db "     Type of Address = ", 0
msgApicId           db "APIC ID: ", 0
msgApicVersion      db "APIC Version: ", 0
msgError            db "Error!!!", 0

msgFoundVesaPmib            db "Found VESA PM Info Block at ", 0
msgVesaPmibNotFound         db "VESA PM Info Block not found!", 0
msgPMInfoBlockSignature     db "Signature: ", 0
msgPMInfoBlockEntryPoint    db "EntryPoint: ", 0
msgPMInfoBlockPMInitialize  db "PMInitialize: ", 0
msgPMInfoBlockBiosDataSel   db "BiosDataSel: ", 0
msgPMInfoBlockA0000Sel      db "A0000Sel: ", 0
msgPMInfoBlockB0000Sel      db "B0000Sel: ", 0
msgPMInfoBlockB8000Sel      db "B8000Sel: ", 0
msgPMInfoBlockCodeSegSel    db "CodeSegSel: ", 0
msgPMInfoBlockInProtectMode db "InProtectMode: ", 0
msgPMInfoBlockChecksum      db "Checksum: ", 0
msgPMInfoBlockSize          db "Size: ", 0

msgVbeInfoBlock                  db "VbeInfoBlockVbe", 0
msgVbeInfoBlockVbeSignature      db "VbeInfoBlockVbeSignature: ", 0
msgVbeInfoBlockVbeVersion        db "VbeInfoBlockVbeVersion: ", 0
msgVbeInfoBlockOemStringPtr      db "VbeInfoBlockOemString: ", 0
msgVbeInfoBlockCapabilities      db "VbeInfoBlockCapabilities: ", 0
msgVbeInfoBlockVideoModePtr      db "VbeInfoBlockVideoModePtr: ", 0
msgVbeInfoBlockTotalMemory       db "VbeInfoBlockTotalMemory: ", 0
msgVbeInfoBlockOemSoftwareRev    db "VbeInfoBlockOemSoftware: ", 0
msgVbeInfoBlockOemVendorNamePtr  db "VbeInfoBlockOemVendorName: ", 0
msgVbeInfoBlockOemProductNamePtr db "VbeInfoBlockOemProductName: ", 0
msgVbeInfoBlockOemProductRevPtr  db "VbeInfoBlockOemProductRev: ", 0

msgGetVbeControllerInfoResult   db "GetVbeControllerInfo result: ", 0
GetVbeControllerInfoResult      dw 0ffffh

msgRAX                          db  "RAX: ", 0

VbeInfoBlock
VbeInfoBlock.VbeSignature      times 4   db 0
VbeInfoBlock.VbeVersion                  dw 0
VbeInfoBlock.OemStringPtr                dd 0
VbeInfoBlock.Capabilities      times 4   db 0
VbeInfoBlock.VideoModePtr                dd 0
VbeInfoBlock.TotalMemory                 dw 0
VbeInfoBlock.OemSoftwareRev              dw 0
VbeInfoBlock.OemVendorNamePtr            dd 0
VbeInfoBlock.OemProductNamePtr           dd 0
VbeInfoBlock.OemProductRevPtr            dd 0
VbeInfoBlock.Reserved          times 222 db 0
VbeInfoBlock.OemData           times 256 dw 0
VbeInfoBlock.Size                        EQU($ - VbeInfoBlock)


; Contains pages for first 512 KiB.  Indicates regions that have been used
; for system data or functions.
OriginalMemoryBitmap    dd 0, 0, 0, 0  ; 4 * 32 * 4906

fnGetFile               dd 0
fnStart                 dd 0

ElfoEntryPoint          dq 0
MemoEntryPoint          dq 0

ElfoSize                dq 0
MemoSize                dq 0

Elfo                    db "ELFO       "
Memo                    db "MEMO       "

