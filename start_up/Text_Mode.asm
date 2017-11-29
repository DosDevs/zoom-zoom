; ---------------------------------------------------------------
; ------- Copyright 2015 Daniel Valencia, All Rights Reserved. --
; -------                                                      --
; ------- Term_Driver.asm                                      --
; -------                                                      --
; ------- Provides functions to manipulate the text mode       --
; ------- display and properties.                              --
; ---------------------------------------------------------------


[BITS 16]

; ---------------------------------------------------------
; ------- Print_16:                                      --
; ---------------------------------------------------------

Print_16:
    xor ecx, ecx
print_16_n:
    push eax
    push esi
Print_16.loop:
    lodsb
    cmp al, 0
    je Print_16.done
    call Put_Char_16
    loop Print_16.loop
Print_16.done:
    pop esi
    pop eax
    ret

;Put_Char_16:
    pushad
    mov ah, 0eh
    int 10h
    popad
    ret

 Put_Char_16:
    pushad
    mov bx, 0b800h
    mov es, bx
    mov bx, [Cursor_Pos]
    shl bx, 1
    mov [es:bx], al
    call Advance_Cursor_16
    popad
    ret



; ---------------------------------------------------------
; ------- Set_Cursor_Pos_16:                             --
; ---------------------------------------------------------

Set_Cursor_Pos_16:
    push ax
    push bx

    mov ax, [Cursor_Pos]

    mov bl, al                    ;
    mov al, 0eh                   ; High
    call Write_CRTC_Register_16   ;

    mov ah, bl                    ;
    mov al, 0fh                   ; Low
    call Write_CRTC_Register_16   ;

Set_Cursor_Pos_16.Wait:
    mov ecx, 10000h
    rep nop

    call Read_Cursor_Pos_16
    cmp word [Cursor_Pos], ax
    jne Set_Cursor_Pos_16.Wait

    pop bx
    pop ax
ret


; ---------------------------------------------------------
; ------- Write_CRTC_Register:                           --
; -------                                                --
; -------      IN: al = register                         --
; -------          ah = value                            --
; ---------------------------------------------------------

Write_CRTC_Register_16:
    push ax
    push dx

    mov dx, VGA__CRTC_ADDR
    out dx, al               ; Send Address
    shr ax, 8
    mov dx, VGA__CRTC_DATA
    out dx, al               ; Get Datum

    pop dx
    pop ax
ret





; ---------------------------------------------------------
; ------- Dump                                           --
; ---------------------------------------------------------

Dump16:
    call crlf16
    xor ebx, ebx

Dump16.BeginNewLine:
    push esi
    mov ax, ds
    call Print_WORD_16
    call Colon_16
    mov eax, esi
    call Print_DWORD_16
    call Space16
    call Space16
    pop esi
    loop Dump16.Loop

Dump16.Loop:
    lodsb
    push esi
    call Print_BYTE_16
    call Space16
    inc ebx
    test ebx, 07h
    jz Dump16.Hyphen
    jmp Dump16.DoLoop

Dump16.Hyphen:
    test ebx, 0fh
    jz Dump16.NewLine
    mov al, '-'
    call Put_Char_16
    call Space16
    jmp Dump16.DoLoop

Dump16.NewLine:
    call crlf16
    test ebx, 7fh
    jnz Dump16.BeginNewLine
    call crlf16
    jmp Dump16.BeginNewLine

Dump16.DoLoop:
    pop esi
    loop Dump16.Loop
    call crlf16
    ret


crlf16:
    pushad
    mov ax, [Cursor_Pos]
    sub ax, [Cursor_Col]
    add ax, word [Text_Columns]
    mov word [Cursor_Pos], ax
    mov word [Cursor_Col], 0
    inc word [Cursor_Row]
    call Set_Cursor_Pos_16
    popad
    ret

Dot16:
    mov al, '.'
    jmp Put_Char_16

Colon_16:
    mov al, ':'
    jmp Put_Char_16

Comma_16:
    mov al, ','
    jmp Put_Char_16

Space16:
    mov al, ' '
    jmp Put_Char_16


;; ---------------------------------------------------------
;; ------- Init_Term:                                     --
;; ---------------------------------------------------------
;    
;
;Init_Term:
;    call Init_Video
;ret
;
;
;    
;
;
;
;; ---------------------------------------------------------
;; ---------------------------------------------------------
;; ------- Video_Driver                                   --
;; ---------------------------------------------------------
;; ---------------------------------------------------------
;
;Video_Driver:
;
;; ---------------------------------------------------------
;; ------- Print_Char:                                    --
;; -------                                                --
;; -------      AL = Character                            --
;; ---------------------------------------------------------
;
;Print_Char:
;    movzx ebx, word [Cursor_Pos]
;    lea ebx, [ebx*2+VGA__BASE_ADDRESS]
;    mov [ebx], al
;    mov al, [Default_Attribute]
;    inc ebx
;    mov [ebx], al
;    call Advance_Cursor
;ret
;
;
;
;
;
;
;; ---------------------------------------------------------
;; ------- Print_String:                                  --
;; -------                                                --
;; -------      RSI = Address of First Character          --
;; ---------------------------------------------------------
;
;Print_String:
;    lodsb
;    cmp al, 0
;    je Print_String.Done
;    cmp al, '\'
;    je Print_String.Special
;    call Print_Char
;    jmp Print_String
;
;Print_String.Done:
;    ret
;
;Print_String.Special:
;    lodsb
;    cmp al, 'b'
;    jne Print_String.Special.1
;    call Back_Space
;    jmp Print_String
;
;Print_String.Special.1:
;    jmp Print_String



; ---------------------------------------------------------
; ------- Print_N:                                       --
; ---------------------------------------------------------

Print_DWORD_16:
    push eax
    push ecx
	mov ecx, 8
    jmp Print_UINT_16.Loop

Print_WORD_16:
    push eax
    push ecx
    mov ecx, 4
    shl eax, 16
    jmp Print_UINT_16.Loop

Print_BYTE_16:
    push eax
    push ecx
    mov ecx, 2
    shl eax, 24
    jmp Print_UINT_16.Loop

Print_UINT_16.Loop:
    rol eax, 4
    push eax
    call Print_Digit_16
    pop eax
    loop Print_UINT_16.Loop
    pop ecx
    pop eax
    ret
    
Print_Digit_16:
    and al, 0fh
    cmp al, 9
    ja Print_Digit_16.Alpha
    or al, 30h

Print_Digit_16.PutChar:
    call Put_Char_16
    ret

Print_Digit_16.Alpha:
    add al, 40h - 9
    jmp Print_Digit_16.PutChar



; ---------------------------------------------------------
; ------- Special Characters                             --
; ---------------------------------------------------------


    ;
    ; New_Line ('\n'):
    ;

New_Line_16:
    pushad
    call Carriage_Return_16

    mov al, byte [Cursor_Row]
    mov bl, byte [Text_Rows]
    inc al
    cmp al, bl
    jb New_Line_16.NoScroll

    mov cl, byte [Text_Columns]
    dec al
    mul cl
    mov bx, 0b800h
    mov ds, bx
    mov es, bx
    mov si, [Text_Columns]
    shl bx, 1
    movzx ecx, ax
    shr ecx, 1
    xor edi, edi
    rep movsd
    movzx ecx, byte [Text_Columns]
    shr ecx, 1
    xor eax, eax
    rep stosd
    jmp New_Line_16.Done

New_Line_16.NoScroll:
    inc byte [Cursor_Row]
    movzx bx, byte [Text_Columns]
    add word [Cursor_Pos], bx

New_Line_16.Done:
    popad
ret


    ;
    ; Carriage_Return ('\r'):
    ;

Carriage_Return_16:
    push ax
    push bx

    mov ax, [Cursor_Pos]
    movzx bx, byte [Cursor_Col]
    sub ax, bx
    mov [Cursor_Pos], ax

    mov byte [Cursor_Col], 0

    pop bx
    pop ax
ret


    ;
    ; Back_Space ('\b'):
    ;

Back_Space_16:
;    call Retreat_Cursor
ret






; ---------------------------------------------------------
; ------- Calculate_Cursor_Pos:                          --
; ---------------------------------------------------------

Calculate_Cursor_Pos_16:
    mov al, [Cursor_Row]
    mul byte [Text_Columns]
    add al, [Cursor_Col]
    adc ah, 0
    mov [Cursor_Pos], ax
ret






; ---------------------------------------------------------
; ------- Advance_Cursor:                                --
; ---------------------------------------------------------

Advance_Cursor_16:
    mov al, [Cursor_Col]
    inc al
    cmp al, [Text_Columns]
    jb Advance_Cursor_16.Same_Line
    call New_Line_16
    jmp Advance_Cursor_16.Done

Advance_Cursor_16.Same_Line:
    mov byte [Cursor_Col], al
    inc word [Cursor_Pos]

Advance_Cursor_16.Done:
    ret






;; ---------------------------------------------------------
;; ------- Retreat_Cursor:                                --
;; ---------------------------------------------------------
;
;Retreat_Cursor:
;    mov al, [Cursor_Col]
;    cmp al, 0
;    jna Retreat_Cursor.End
;    dec al
;    mov [Cursor_Col], al
;    mov ax, [Cursor_Pos]
;    dec ax
;
;Retreat_Cursor.End:
;    call Set_Cursor_Pos
;ret
;
;
;
;
;
;
; ---------------------------------------------------------
; ------- Read_CRTC_Register_16                          --
; -------                                                --
; -------      IN: al = register                         --
; -------     OUT: al = value                            --
; ---------------------------------------------------------

Read_CRTC_Register_16:
    mov dx, VGA__CRTC_ADDR
    out dx, al               ; Send Address
    mov dx, VGA__CRTC_DATA
    in al, dx                ; Get Datum
ret








; ---------------------------------------------------------
; ------- Read_MISC_Register:                            --
; -------                                                --
; -------     OUT: al = value                            --
; ---------------------------------------------------------

Read_MISC_Register:
    mov dx, VGA__MISC_READ
    in al, dx
ret





; ---------------------------------------------------------
; ------- Write_MISC_Register:                            --
; -------                                                --
; -------     OUT: al = value                            --
; ---------------------------------------------------------

Write_MISC_Register:
    mov dx, VGA__MISC_WRITE
    out dx, al
ret





; ---------------------------------------------------------
; ------- Init_Video:                                    --
; ---------------------------------------------------------
    

Init_Video:

         ;
         ; Make sure we are using 3Dx addresses
         ;
    call Read_MISC_Register
    or al, 1
    call Write_MISC_Register

    call Read_Cursor_Pos_16
    mov [Cursor_Pos], ax


         ;
         ; Translate Cursor Position into Row/Col
         ;
    div byte [Text_Columns]       ; Divide by number of columns
    mov [Cursor_Row], al   ; Save Row (quotient)
    mov [Cursor_Col], ah   ; Save Column (remainder)

    call crlf16
ret

Read_Cursor_Pos_16:
         ;
         ; Read Cursor Position
         ;
    mov al, 0eh                   ;
    call Read_CRTC_Register_16    ; High
    shl ax, 8                     ;
    mov al, 0fh                   ;
    call Read_CRTC_Register_16    ; Low
    ret                           ;


; ---------------------------------------------------------
; ------- 64-bit version of the functions.               --
; ---------------------------------------------------------


[BITS 64]

; ---------------------------------------------------------
; ------- Dump                                           --
; ---------------------------------------------------------

Dump.Count dq 0
Dump.Ptr dq 0
Dump.End dq 0

Dump:
    mov word [Dump.Count], 0
    mov [Dump.Ptr], rsi
    add rcx, rsi
    mov [Dump.End], rcx

Dump.BeginNewLine:
    mov rax, [Dump.Ptr]
    cmp rax, [Dump.End]
    jge Dump.Loop.End
    call Print_DWORD
    loop Dump.Loop

Dump.Loop:
	mov rbx, [Dump.Ptr]
    cmp rbx, [Dump.End]
    jge Dump.Loop.End
    call Space
    inc dword [Dump.Ptr]
    mov al, [rbx]
    call Print_BYTE
    inc dword [Dump.Count]
    test dword [Dump.Count], 07h
    jz Dump.Hyphen
    jmp Dump.Loop

Dump.Hyphen:
    test dword [Dump.Count], 0fh
    jz Dump.NewLine
    call Space
    mov al, '-'
    call Put_Char
    jmp Dump.Loop

Dump.NewLine:
    mov rax, [Dump.Ptr]
    cmp rax, [Dump.End]
    jge Dump.Loop.End
    call New_Line
    test dword [Dump.Count], 7fh
    jnz Dump.BeginNewLine
    call New_Line
    jmp Dump.BeginNewLine

Dump.Loop.End:
    call New_Line
    ret


Dot:
    mov al, '.'
    jmp Put_Char

Colon:
    mov al, ':'
    jmp Put_Char

Space:
    mov al, ' '
    jmp Put_Char


; ---------------------------------------------------------
; ------- Show_N:                                        --
; ---------------------------------------------------------

;Show_String_N:
;    push ecx
;    xor ecx, ecx
;    call print_n
;    mov esi, eax
;    pop ecx
;    call Puts
;    ret

Show_String:
    call Print
    mov esi, eax
    call Puts
    ret

Show_QWORD:
    call Print
    call Print_QWORD
    call New_Line
    ret

Show_DWORD:
    call Print
    call Print_DWORD
    call New_Line
    ret

Show_WORD:
    call Print
    call Print_WORD
    call New_Line
    ret

Show_BYTE:
    call Print
    call Print_BYTE
    call New_Line
    ret



; ---------------------------------------------------------
; ------- Puts:                                          --
; -------                                                --
; -------      RSI = Address of First Character          --
; ---------------------------------------------------------

Puts:
    call Print
    call New_Line
ret


; ---------------------------------------------------------
; ------- Print_N:                                       --
; ---------------------------------------------------------

Print_QWORD:
    push rax
    push rcx
    mov rcx, 16
    jmp Print_UINT.Loop

Print_DWORD:
    push rax
    push rcx
    mov rcx, 8
    shl rax, 32
    jmp Print_UINT.Loop

Print_WORD:
    push rax
    push rcx
    mov rcx, 4
    shl rax, 48
    jmp Print_UINT.Loop

Print_BYTE:
    push rax
    push rcx
    mov rcx, 2
    shl rax, 56
    jmp Print_UINT.Loop

Print_UINT.Loop:
    rol rax, 4
    push rax
    call Print_Digit
    pop rax
    loop Print_UINT.Loop
    pop rcx
    pop rax
    ret
    
Print_Digit:
    and al, 0fh
    cmp al, 9
    ja Print_Digit.Alpha
    or al, 30h

Print_Digit.PutChar:
    call Put_Char
    ret

Print_Digit.Alpha:
    add al, 40h - 9
    jmp Print_Digit.PutChar



; ---------------------------------------------------------
; ------- Print:                                         --
; ---------------------------------------------------------

Print:
    push rax
    push rsi
Print.loop:
    lodsb
    cmp al, 0
    je Print.done
    call Put_Char
    jmp Print.loop
Print.done:
    pop rsi
    pop rax
    ret

Put_Char:
    push rbx
    push rcx

    mov rbx, VGA__BASE_ADDRESS
    movzx rcx, word [Cursor_Pos]
    shl cx, 1
    add rbx, rcx
    mov [rbx], al

    call Advance_Cursor
    pop rcx
    pop rbx
    ret


    ;
    ; New_Line ('\n'):
    ;

New_Line:
    push ax
    
    mov ax, [Cursor_Pos]
    sub ax, [Cursor_Col]
    mov [Cursor_Pos], ax
    mov word [Cursor_Col], 0
    mov ax, [Cursor_Row]
    inc ax
    cmp ax, [Text_Rows]
    je Scroll_Up
    mov [Cursor_Row], ax

    mov ax, [Cursor_Pos]
    add ax, [Text_Columns]
    mov [Cursor_Pos], ax

New_Line.Done:
    call Set_Cursor_Pos
    pop ax
ret

Scroll_Up:
	dec ax
	mov [Cursor_Row], ax

    mov rsi, VGA__BASE_ADDRESS + 160
    mov rdi, VGA__BASE_ADDRESS
    mov rcx, 480
    rep movsq
    mov rax, 2a002a002a002a00h
    mov rcx, 20
    rep stosq
	jmp New_Line.Done

Comma:
    mov al, ','
    jmp Put_Char


    ;
    ; Carriage_Return ('\r'):
    ;

Carriage_Return:
    push ax
    push bx

    mov ax, [Cursor_Pos]
    movzx bx, byte [Cursor_Col]
    sub ax, bx
    mov [Cursor_Pos], ax

    mov byte [Cursor_Col], 0

    pop bx
    pop ax
ret


; ---------------------------------------------------------
; ------- Advance_Cursor:                                --
; ---------------------------------------------------------

Advance_Cursor:
    push ax
    mov ax, [Cursor_Col]
    inc ax
    cmp ax, [Text_Columns]
    jb Advance_Cursor.Same_Line
    dec ax
    call New_Line
    jmp Advance_Cursor.Done

Advance_Cursor.Same_Line:
    mov [Cursor_Col], ax
    inc word [Cursor_Pos]

Advance_Cursor.Done:
    call Set_Cursor_Pos
    pop ax
    ret






; ---------------------------------------------------------
; ------- Set_Cursor_Pos   :                             --
; ---------------------------------------------------------

Set_Cursor_Pos:
    push ax
    push bx

    mov ax, [Cursor_Pos]

    mov bl, al                    ;
    mov al, 0eh                   ; High
    call Write_CRTC_Register      ;

    mov ah, bl                    ;
    mov al, 0fh                   ; Low
    call Write_CRTC_Register      ;

Set_Cursor_Pos.Wait:
    mov rcx, 10000h
    rep nop

    call Read_Cursor_Pos
    cmp word [Cursor_Pos], ax
    jne Set_Cursor_Pos.Wait

    pop bx
    pop ax
ret

Read_Cursor_Pos:
         ;
         ; Read Cursor Position
         ;
    mov al, 0eh                   ;
    call Read_CRTC_Register       ; High
    shl ax, 8                     ;
    mov al, 0fh                   ;
    call Read_CRTC_Register       ; Low
    ret                           ;


; ---------------------------------------------------------
; ------- Write_CRTC_Register:                           --
; -------                                                --
; -------      IN: al = register                         --
; -------          ah = value                            --
; ---------------------------------------------------------

Write_CRTC_Register:
    push rax
    push rdx

    mov dx, VGA__CRTC_ADDR
    out dx, al               ; Send Address.
    shr ax, 8
    mov dx, VGA__CRTC_DATA
    out dx, al               ; Send Datum.

    pop rdx
    pop rax
ret

; ---------------------------------------------------------
; ------- Read_CRTC_Register:                            --
; -------                                                --
; -------      IN: al = register                         --
; -------     OUT: al = value                            --
; ---------------------------------------------------------

Read_CRTC_Register:
    mov dx, VGA__CRTC_ADDR
    out dx, al               ; Send Address
    mov dx, VGA__CRTC_DATA
    in al, dx                ; Get Datum
ret


; ---------------------------------------------------------
; ------- kprint (C calling convention):                 --
; ---------------------------------------------------------

kprint:
    push rbp
    mov rbp, rsp
    push rbx
    push rsi
    push rdi

    mov rsi, rdi
    call Print

    pop rdi
    pop rsi
    pop rbx
    mov rsp, rbp
    pop rbp
    xor rax, rax
ret


; ---------------------------------------------------------
; ------- kputchar (C calling convention):               --
; ---------------------------------------------------------

kputchar:
    push rbp
    mov rbp, rsp
    push rbx
    push rsi
    push rdi

    mov rax, rdi  ; We just care about what's in AL.
    call Put_Char

    pop rdi
    pop rsi
    pop rbx
    mov rsp, rbp
    pop rbp
    xor rax, rax
ret


; ---------------------------------------------------------
; ------- knewline (C calling convention):               --
; ---------------------------------------------------------

knewline:
    push rbp
    mov rbp, rsp
    push rbx
    push rsi
    push rdi

    call New_Line

    pop rdi
    pop rsi
    pop rbx
    mov rsp, rbp
    pop rbp
    xor rax, rax
ret


; ---------------------------------------------------------
; ------ ksetcursor (C calling convention):              --
; ---------------------------------------------------------

ksetcursor:
    push rbp
    mov rbp, rsp
    push rbx
    push rsi
    push rdi

    mov [Cursor_Pos], di
    call Set_Cursor_Pos

    pop rdi
    pop rsi
    pop rbx
    mov rsp, rbp
    pop rbp
    xor rax, rax
ret

; ---------------------------------------------------------
; ------ kgetcursor (C calling convention):              --
; ---------------------------------------------------------

kgetcursor:
    push rbp
    mov rbp, rsp
    push rbx
    push rsi
    push rdi

    xor rax, rax

    call Read_Cursor_Pos
    mov [Cursor_Pos], ax

    pop rdi
    pop rsi
    pop rbx
    mov rsp, rbp
    pop rbp
ret


VGA__CRTC_ADDR      EQU 03d4h
VGA__CRTC_DATA      EQU 03d5h
VGA__MISC_READ      EQU 03cch
VGA__MISC_WRITE     EQU 03c2h
VGA__BASE_ADDRESS   EQU 0b8000h

Text_Positions      dw 2000
Text_Columns        dw 80
Text_Rows           dw 25
Cursor_Pos          dw 0
Cursor_Row          dw 0
Cursor_Col          dw 0
Cursor_Scan_Start   db 0
Cursor_Scan_End     db 0
Default_Attribute   db 7

msgX                db "XXXXXXXXXXXXXX", 0
