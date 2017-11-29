; ---------------------------------------------------------------
; ------- Copyright 2010 Daniel Valencia; All Rights Reserved. --
; -------                                                      --
; ------- bs.asm                                               --
; -------                                                      --
; ------- A Boot Loader for FAT12 Filesystems                  --
; ---------------------------------------------------------------

[BITS 16]
jmp short Start       ; Go to the REAL entry point.
nop

OEM_Name              db "ZoomZoom"

Sector_Size           dw 0
Cluster_Sectors       db 0
Reserved_Sectors      dw 0
FAT_Copies            db 0
RD_Entries            dw 0
Volume_Sectors        dw 0
Medium_Type           db 0
FAT_Sectors           dw 0
Track_Sectors         dw 0
Drive_Heads           dw 0
Hidden_Sectors        dw 0


; ---------------------------------------------------------
; ------- Start: The REAL entry point.                   --
; -------                                                --
; ------- Purpose:                                       --
; -------   To set up the main registers                 --
; -------   for the next steps.                          --
; -------                                                --
; -------          Registers            Variables        --
; -------                                                --
; ------- Input:   DL = bootdrive       None             --
; -------                                                --
; ------- Output:  None                 None             --
; ---------------------------------------------------------

Start:
     xor ax, ax
     mov ds, ax
     mov es, ax
     mov ss, ax
     mov sp, ax
     mov esi, 7c00h
     mov edi, BS_ADDRESS
     mov ecx, 80h
     rep movsd
     jmp 0:Start_1

[ORG BS_ADDRESS]
Start_1:
     mov [bootdrive], dl
     call Enable_A20
     lea esi, [Image]
     mov bx, START_ADDRESS / 10h
     call Get_File

    ;
    ; First, find the file in the filesystem.
    ;


Leap:
     push dword Get_File       ; Publish Service
     push dword START_ADDRESS  ; Inform of starting address
     jmp 0:START_ADDRESS

Error:
     hlt


; ---------------------------------------------------------
; ------- Enable_A20:                                    --
; -------   Disables the masking of A20 that was put     --
; -------   as a backwards-compatibility hack.           --
; ---------------------------------------------------------

Enable_A20:
     call Enable_A20.Wait          ; Spin
     mov al, 0xd1
     out 0x64, al
     call Enable_A20.Wait          ; Spin
     mov al, 0xdf
     out 0x60, al
     call Enable_A20.Wait          ; Spin
     ret

Enable_A20.Wait:
     bts ecx, 16
     rep nop
     in al, 0x64
     test al, 2
     jnz Enable_A20.Wait
ret


; ------------------------------------------------------------
; ------- Find_File:                                        --
; -------   Inputs:                                         --
; -------      ESI - Zero-terminated string with filename.  --
; -------   Read_Root                                       --
; ------------------------------------------------------------

FindFile:
    ; Load one sector of the FAT at a time until
    ; you have either found the file or run out of
    ; FAT sectors to load.

;   ; ---------------------------------------------------------
;   ; ------- Read_Stuff:                                    --
;   ; -------   Compute_Stuff                                --
;   ; -------   Read_FAT                                     --
;   ; -------   Read_Root                                    --
;   ; ---------------------------------------------------------
;   
;   Read_Stuff:
;   Compute_Stuff:
;        ; Compute Cylinder_Sectors
;        mov ax, word [Track_Sectors]
;        mul word [Drive_Heads]
;        mov byte [Cylinder_Sectors], al ; Should fit in 8 bits.
;   
;        ; Compute Cluster_Paras
;        mov ax, word [Sector_Size]     ; Max. sector size = 2 KiB.
;        shr ax, 4                      ; AX = Paragraphs per sector.
;        mul byte [Cluster_Sectors]     ; AX = paragraphs per cluster.
;        mov word [Cluster_Paras], ax
;   
;        ; Compute Root_Start
;        mov ax, word [FAT_Sectors]    ; Retrieve FAT sectors.
;        mov cx, [Reserved_Sectors]    ; Retrieve Start of FAT.
;        mul byte [FAT_Copies]         ; There are FAT copies.
;        add ax, cx                    ; RD starting sector.
;        mov word [Root_Start], ax     ; Save.
;   
;        ; Compute Root_Sectors
;        movzx eax, word [RD_Entries]
;        shl ax, 5                     ; 32 bytes per RD entry.
;        div word [Sector_Size]        ; AX=Sectors in RootDir.
;        mov word [Root_Sectors], ax   ; Save.
;   
;        ; Compute Data_Start
;        add ax, word [Root_Start]     ; AX=Root_Start+Root_Sectors
;        mov word [Data_Start], ax     ; Save.
;   
;   Read_FAT:
;        mov ax, [Reserved_Sectors]    ; First FAT starts here.
;        push ax
;        mov cx, [FAT_Sectors]         ; Sectors in each FAT.
;        mov bx, FAT_ADDRESS / 10h     ; Load the destination->
;        mov es, bx                    ; -> segment into ES.
;        call Read_Sectors
;   
;   Read_Root:
;        mov ax, word [Root_Start]     ; [Root_Start]
;        pop cx                        ; [Root_Sectors]
;        mov bx, ROOT_ADDRESS / 10h
;        mov es, bx
;        call Read_Sectors
;        ret



; ---------------------------------------------------------
; ------- Get_File:                                      --
; -------   Locates a file by name inside                --
; -------   the Root Directory, and then reads           --
; -------   its contents.                                --
; -------                                                --
; ------- Input: ES:0=Pointer to root directory.         --
; -------        BX:0=Pointer to destination.            --
; -------                                                --
; ------- Output: AX=Number of first cluster.            --
; -------         CX=File size in bytes.                 --
; -------                                                --
; ------- On Error: Hang.                                --
; ---------------------------------------------------------

Get_File:
     mov ax, ROOT_ADDRESS / 10h
     mov es, ax
     mov eax, esi
Find_File:
     mov esi, eax
     xor edi, edi
     cmp byte [edi], 0             ; If Empty
     je Error                      ;    Error
     mov ecx, 11                   ; Else
     repe cmpsb                    ;    Compare 11 Bytes
     je Find_File.Found            ; If Not Found
     mov dx, es
     inc dx
     inc dx
     mov es, dx                    ;    Move To Next Entry
     jmp Find_File                 ;    Repeat

Find_File.Found:
     mov ax, es
     mov ax, word [es:1ah]         ; Initial Cluster
     mov cx, word [es:1ch]         ; File size in byres.

Read_File:
     push cx
     call Read_Cluster
     pop cx
     cmp ax, 0ff0h
     jnb Read_File.Done
     mov bx, es
     add bx, [Cluster_Paras]
     jmp Read_File

Read_File.Done:
     ret


; ---------------------------------------------------------
; ------- Read_Cluster:                                  --
; -------   Reads the contents of a cluster into memory. --
; -------                                                --
; ------- Input: AX = Cluster number.                    --
; -------        BX:0 = Pointer to destination.          --
; -------                                                --
; ------- Output: AX will contain the next               --
; -------         Cluster's number.                      --
; ---------------------------------------------------------

Read_Cluster:
     ; Starting sector = (Cluster - 2) * Cluster_Sectors
     ;                 + Data_Start
     push ax
     mov es, bx
     sub ax, 2
     movzx bx, byte [Cluster_Sectors]
     push bx
     mul bx                        ; DX:AX = AX * BX
     add ax, [Data_Start]
     pop cx                        ; [Cluster_Sectors]
     call Read_Sectors

Next_Cluster:
     ; BX = (Last_Cluster * 3) / 2
     pop ax
     mov bx, ax
     shl bx, 1
     add bx, ax
     mov cx, cs
     shr bx, 1

     ; CX = CF (is Last_Cluster odd?)
     adc cx, cx
     mov ax, [bx+FAT_ADDRESS] ; Read Little-Endian Word.
     shr cx, 1
     jc Next_Cluster.Odd      ; If Even Cluster
     and ax, 0fffh            ; Chop last nibble.
     jmp Next_Cluster.Done

Next_Cluster.Odd:             ; Else
     shr ax, 4                ; Chop first nibble.

Next_Cluster.Done:
     ret



; ---------------------------------------------------------
; ------- Read_Sectors:                                  --
; -------   Reads a number of consecutive sectors from   --
; -------   disk into memory.                            --
; -------                                                --
; ------- Input: CL=Number of sectors to read.           --
; -------        AX=Number of starting sector.           --
; -------                                                --
; ------- Output: ES:0=pointer to contents in memory.    --
; ---------------------------------------------------------

Read_Sectors:
     mov ch, 2           ; Function Number, will end up in ah
     push cx             ; Save FN and Number of Sectors

          ; Convert LBA to CHS
     mov bl, [Cylinder_Sectors]
     div bl
     mov ch, al          ; Save Cylinder Number in CH
     shr ax, 8           ; Shift AH into AL so that AX has the reminder
     mov bl, byte [Track_Sectors]
     div bl
     mov dh, al          ; Save Head Number in DH
     mov cl, ah          ; Save Sector Number in CL
     inc cl

          ; Make the call
     mov bx, cs          ; Clear destination offset
     mov dl, [bootdrive] ; Set source drive
     pop ax              ; Retrieve FN and NoS
     int 13h
ret



; ---------------------------------------------------------
; -------                   D a t a                      --
; ---------------------------------------------------------

NULL_SEGMENT     EQU 0000h
PM_CODE_SEGMENT  EQU 0008h
LM_CODE_SEGMENT  EQU 0010h
DATA_SEGMENT     EQU 0018h

BS_ADDRESS       EQu 0600h
FAT_ADDRESS      EQU 2000h
ROOT_ADDRESS     EQU 4000h
START_ADDRESS    EQU 6000h

Image            db "START      "

Cylinder_Sectors db 0
Cluster_Paras    dw 0
Root_Sectors     dw 0
Root_Start       dw 0
Data_Start       dw 0
bootdrive        db 0

;fwd dd 0
;bwd dd 0


; ---------------------------------------------------------
; -------                  M a g i c                     --
; ---------------------------------------------------------

; times 1feh-($-$$) db 0         ; 07dfeh
db 055h, 0aah

