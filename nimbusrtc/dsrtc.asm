; MS-DOS CLOCK$ device driver for David's RTC board on the RM Nimbus PC-186
; nasm -dDEBUG -l dsrtc.lst -o dsrtc.sys dsrtc.asm
                [warning -zeroing]
                [warning -label-orphan]
                cpu 186

; Macros for debugging output
%macro          dbgchr 1
%ifdef          DEBUG
                push dx
                push ax
                mov dl,'['
                mov ah,2
                int 0x21
                mov dl,%1
                mov ah,2
                int 0x21
                mov dl,']'
                mov ah,2
                int 0x21
                pop ax
                pop dx
%endif
%endmacro

; Device driver header
header          dd 0xFFFFFFFF
                dw 0x8008                       ; Char device, current CLOCK$
                dw strategy
                dw interrupt
                db "CLOCK$  "

request_off     resw 1                          ; Pointer to request packet
request_seg     resw 1
save_sp         resw 1                          ; Original stack pointer
save_ss         resw 1

; Function table
functbl         dw fn_init                      ; Initialise
                dw fn_nop                       ; Media check
                dw fn_nop                       ; Build BPB
                dw fn_nop                       ; IOCTL input
                dw fn_input                     ; Input (read)
                dw fn_nop                       ; Input, no wait
                dw fn_nop                       ; Input status
                dw fn_nop                       ; Input flush
                dw fn_output                    ; Output (write)
                dw fn_output                    ; Output, verify
                dw fn_nop                       ; Output status
maxfunc         equ (($-functbl)/2)-1

; Local stack
                resb 256
newsp

; Variables
rtcaddr         resw 1                          ; Base I/O address of RTC

; Strategy entry point
strategy        mov word cs:[request_off],bx
                mov word cs:[request_seg],es
                retf

; Interrupt entry point
interrupt       push bx
                mov word cs:[save_sp],sp
                mov word cs:[save_ss],ss
                mov bx,cs
                mov ss,bx
                mov sp,newsp
                push ds
                mov ds,bx
                push di
                push es

                les di,[request_off]
                mov bl,byte es:[di+2]
                cmp bl,maxfunc
                jbe .validfn
                mov bl,maxfunc
.validfn        xor bh,bh
                shl bx,1
                call word [bx+functbl]
                or word es:[di+3],0x0100

                pop es
                pop di
                pop ds
                mov ss,cs:[save_ss]
                mov sp,cs:[save_sp]
                pop bx
                retf

; Read time/date from the RTC
fn_input        push dx
                push cx
                push ax
                push di
                push es

                dbgchr 'I'
                les di,es:[di+0xE]
                cli
                cld
                call inhibit

                mov dx,9                        ; Year
                call rdrtc
                call assumecent
                mov cx,ax
                dec dx                          ; Month
                call rdrtc
                mov ah,al
                dec dx                          ; Day
                call rdrtc
                mov dx,ax
                call datetodays                 ; Days since 01/01/1980
                stosw

                mov dx,2                        ; Minutes
                call rdrtc
                stosb
                mov dx,4                        ; Hours
                call rdrtc
                stosb
                xor al,al                       ; Centiseconds, not implemented
                stosb
                mov dx,0                        ; Seconds
                call rdrtc
                stosb

                call uninhibit
                sti
                dbgchr 'i'

                pop es
                pop di
                pop ax
                pop cx
                pop dx
                ret

; Write time/date to the RTC
fn_output       push dx
                push cx
                push ax
                push si
                push ds

                dbgchr 'O'
                lds si,es:[di+0xE]
                cli
                cld
                call inhibit

                lodsw                           ; Days since 01/01/1980
                call daystodate
                mov ax,cx
                mov cl,100                      ; Remove century part
                div cl
                mov cx,dx
                mov dx,9                        ; Year
                mov al,ah
                call wrrtc
                dec dx                          ; Month
                mov al,ch
                call wrrtc
                dec dx                          ; Day
                mov al,cl
                call wrrtc

                mov dx,2                        ; Minutes
                lodsb
                call wrrtc
                mov dx,4                        ; Hours
                lodsb
                call wrrtc
                lodsb                           ; Centiseconds, not implemented
                mov dx,0                        ; Seconds
                lodsb
                call wrrtc

                call uninhibit
                sti
                dbgchr 'o'

                pop ds
                pop si
                pop ax
                pop cx
                pop dx
                ret

; Do nothing, function not implemented
fn_nop          dbgchr 'X'
                or word es:[di+3],0x8003
                ret

; Set the SET bit in control register B, inhibiting update transfer
; Exit:  DX, AL changed
inhibit         mov dx,0xB
                call rdrtc
                or al,0x80
                call wrrtc
                ret

; Clear the SET bit in control register B, resuming update transfer
; Exit:  DX, AL changed
uninhibit       mov dx,0xB
                call rdrtc
                and al,0x7F
                call wrrtc
                ret

; Read a byte from the RTC, compensating for the rotated bus
; Entry: DX = register address within RTC
; Exit:  AL = byte read
;        AH unchanged
rdrtc           push dx
                shl dx,1
                add dx,cs:[rtcaddr]
                in al,dx
                ror al,1
                pop dx
%ifdef          DEBUG
                call dodbgin
%endif
                ret

; Write a byte to the RTC, compensating for the rotated bus
; Entry: DX = register address within RTC
;        AL = byte to write
wrrtc           push dx
                push ax
                shl dx,1
                add dx,cs:[rtcaddr]
                rol al,1
                out dx,al
                pop ax
                pop dx
%ifdef          DEBUG
                call dodbgout
%endif
                ret

; Table of the number of days from 1/1 to each month
getdaystbl      dw 0                            ; Days from 1/1 to 1/1
                dw 31                           ; Days from 1/1 to 1/2
                dw 31+28                        ; Days from 1/1 to 1/3
                dw 31+28+31                     ; Days from 1/1 to 1/4
                dw 31+28+31+30
                dw 31+28+31+30+31
                dw 31+28+31+30+31+30
                dw 31+28+31+30+31+30+31
                dw 31+28+31+30+31+30+31+31
                dw 31+28+31+30+31+30+31+31+30
                dw 31+28+31+30+31+30+31+31+30+31
                dw 31+28+31+30+31+30+31+31+30+31+30
;                   J  F  M  A  M  J  J  A  S  O  N

; Assume the century part of a 2-digit year
; Entry: AL = 2-digit year
; Exit:  AX = 4-digit year, 20xx if xx00-xx79, 19xx if xx80-xx99
assumecent      xor ah,ah
                cmp al,80
                jnc .19xx
                add ax,100                      ; Assume 20xx if xx00-xx79
.19xx           add ax,1900                     ; Assume 19xx if xx80-xx99
                ret

; Test whether a year is a leap year
; Entry: AX = 4-digit year
; Exit:  Carry set if leap year, clear if not
isleap          push ax
                push bx
                test al,3                       ; Divisible by 4?
                jnz .no
                mov bl,100                      ; Divisible by 100?
                div bl
                and ah,ah
                jnz .yes
                test al,3                       ; Divisible by 400?
                jnz .no
.yes            stc
.no             pop bx
                pop ax
                ret

; Convert day/month/year to number of days since 01/01/1980
; Entry: CX = 4-digit year
;        DH = month (1-12)
;        DL = day (1-31)
; Exit:  AX = number of days (0 means 01/01/1980)
datetodays      push bx
                xor bx,bx                       ; Running result here

                mov ax,cx                       ; Year
                call isleap
                jnc .yrlp
                cmp dh,3                        ; Month
                jc .yrlp
                inc bx                          ; Count this year's leap day

.yrlp           cmp ax,1980                     ; Finished counting full years?
                jz .yrdn
                dec ax
                call isleap
                jnc .noleap2
                inc bx                          ; Count leap day in prev year
.noleap2        add bx,365                      ; Count days in prev year
                jmp .yrlp

.yrdn           xor ax,ax
                mov al,dl                       ; Day of month
                dec ax
                add bx,ax                       ; Count days so far this month

                xor ax,ax
                mov al,dh                       ; Month
                dec ax                          ; Convert to index into table
                shl ax,1
                xchg ax,bx                      ; AX = days, BX = index
                add ax,cs:[bx+getdaystbl]       ; Count prev months this year

                pop bx
                ret

; Convert number of days since 01/01/1980 to day/month/year
; Entry: AX = number of days (0 means 01/01/1980)
; Exit:  CX = 4-digit year
;        DH = month (1-12)
;        DL = day (1-31)
daystodate      push ax
                push bx

                mov cx,ax                       ; CX will contain no. days
                mov ax,1980                     ; AX will count the year
.nxtyr          mov bx,365                      ; How many days in that year?
                call isleap
                jnc .noleap
                inc bx
.noleap         sub cx,bx                       ; Past the end of that year?
                jc .gotyr
                inc ax                          ; Count to the next year
                jmp .nxtyr
.gotyr          add cx,bx                       ; Undo SUB, get back no. days

                call isleap
                xchg ax,cx                      ; CX = year, AX = no. days
                jnc .noleap2
                cmp ax,59                       ; 29 Feb, in a leap year?
                jc .noleap2                     ; < 29 Feb
                jnz .isleap2                    ; > 29 Feb
                mov dh,2                        ; DH = month
                mov dl,29                       ; DL = day
                jmp .rtn
.isleap2        dec ax                          ; Correct for the extra day

.noleap2        mov bx,24                       ; Start at the end of the table
.nxtmth         dec bx                          ; Each item being 1 word
                dec bx
                cmp ax,cs:[bx+getdaystbl]       ; Day count reaches that month?
                jc .nxtmth
                sub ax,cs:[bx+getdaystbl]       ; Leaving days since the 1st
                inc ax                          ; And we now have day-of-month
                shr bx,1                        ; Convert index to month
                inc bx
                mov dh,bl                       ; DH = month
                mov dl,al                       ; DL = day

.rtn            pop bx
                pop ax
                ret

%ifdef          DEBUG
; Produce debugging output
dodbgin         push ax
                push dx

                push ax
                push dx
                mov dl,'['
                mov ah,2
                int 0x21
                pop ax
                call outhw
                mov dl,'r'
                mov ah,2
                int 0x21
                pop ax
                call outhb
                mov dl,']'
                mov ah,2
                int 0x21

                pop dx
                pop ax
                ret

dodbgout        push ax
                push dx

                push ax
                push dx
                mov dl,'['
                mov ah,2
                int 0x21
                pop ax
                call outhw
                mov dl,'w'
                mov ah,2
                int 0x21
                pop ax
                call outhb
                mov dl,']'
                mov ah,2
                int 0x21

                pop dx
                pop ax
                ret

; Output four hex digits to the console
; Entry: AX = unsigned 16-bit integer
outhw           push ax
                mov al,ah
                call outhb
                pop ax

; Output two hex digits to the console
; Entry: AX = an integer, the low byte of which will be output
outhb           push ax
                shr al,4
                call outhn
                pop ax

; Output a single hex digit to the console
; Entry: AX = an integer, the low nibble of which will be output
outhn           push ax
                push dx
                and al,0xF
                cmp al,0xA
                jc .dig
                add al,('A'-0xA)-'0'
.dig            add al,'0'
                mov dl,al
                mov ah,2
                int 0x21
                pop dx
                pop ax
                ret

; Convert a word from binary to BCD
; Entry: AX = unsigned 16-bit integer
; Exit:  AX = BCD equivalent
bintobcd        push dx
                mov dx,ax
                xor ax,ax
.tth            cmp dx,10000
                jc .th
                sub dx,10000
                jmp .tth
.th             cmp dx,1000
                jc .h
                sub dx,1000
                add ax,0x1000
                jmp .th
.h              cmp dx,100
                jc .t
                sub dx,100
                add ax,0x0100
                jmp .h
.t              cmp dx,10
                jc .u
                sub dx,10
                add ax,0x0010
                jmp .t
.u              add ax,dx
                pop dx
                ret
%endif

; Code below here is only used during initialisation, then can be overwritten
fn_init         pusha
                dbgchr 'A'
                lea dx,[msg_welcome]
                mov ah,9
                int 0x21

                dbgchr '1'
                call probertc
                jnc .nortc
                dbgchr '2'

                call chkrtcset                  ; RTC in correct mode?
                jc .setok
                lea dx,[msg_setme]
                mov ah,9
                int 0x21

.setok          mov dx,0xD                      ; Battery alive?
                call rdrtc
                rol al,1
                jc .batok
                lea dx,[msg_lobatt]
                mov ah,9
                int 0x21

.batok          lea dx,[fn_init]                ; Init code can be overwritten
                mov word es:[di+0xE],dx
                mov word es:[di+0x10],cs
                dbgchr '3'

                mov dx,0xA                      ; Control register A
                mov ax,0x20
                call wrrtc
                inc dx                          ; Control register B
                mov ax,0x06
                call wrrtc
                inc dx                          ; Control register C
                call rdrtc
                inc dx                          ; Control register D
                call rdrtc

                lea dx,[msg_yes1]
                mov ah,9
                int 0x21
                mov dx,[rtcaddr]                ; Get slot number from address
                shr dx,7
                and dl,7
                add dl,'0'
                mov ah,2
                int 0x21
                lea dx,[msg_yes2]
.msgrtn         mov ah,9
                int 0x21

                dbgchr 'a'
                popa
                ret

.nortc          lea dx,[header]                 ; Everything can be overwritten
                mov word es:[di+0xE],dx
                mov word es:[di+0x10],cs
                lea dx,[msg_no]
                jmp .msgrtn

; Probe whether there may be a compatible RTC at the current location
; Entry: [rtcaddr] = I/O address to probe
; Exit:  Carry set if found, clear if not found
;        DX, AL changed
isrtchere       xor dx,dx                       ; Seconds
                call rdrtc
                and al,0x80
                jnz .no

                mov dx,0xC                      ; Control C
                call rdrtc
                and al,0x0F
                jnz .no
                dec al                          ; Register should be read only
                call wrrtc
                call rdrtc
                and al,0x0F
                jnz .no

                inc dx                          ; Control D
                call rdrtc
                and al,0x7F
                jnz .no
                dec al                          ; Register should be read only
                call wrrtc
                call rdrtc
                and al,0x7F
                jnz .no

                stc                             ; Success; carry set
.no             ret

; Probe all card selects for an RTC
; Exit:  Carry set if found, clear if not found
;        [rtcaddr] = I/O address
;        DX, BX, AL changed
probertc        mov bx,0x400
.lp             mov [rtcaddr],bx
                call isrtchere
                jc .yes
                add bx,0x80
                cmp bx,0x680
                jc .lp
.yes            ret

; Check the settings in control registers A and B
; Exit:  Carry set if settings are as we want them, clear if not
;        DX, AL changed
chkrtcset       mov dx,0xA                      ; Divider bits set OK?
                call rdrtc
                and al,0x70
                cmp al,0x20                     ; Divider is 010
                jnz .bad
                inc dx                          ; Data mode bits set OK?
                call rdrtc
                and al,0x06
                cmp al,0x06                     ; Binary, 24h clock
                jnz .bad
                stc
.bad            ret

msg_welcome     db 0x0D,0x0A,"Device driver for Dallas-compatible RTC"
                db 0x0D,0x0A,"(c)2024 David Knoll",0x0D,0x0A,'$'
msg_yes1        db "RTC found at card select $"
msg_no          db "RTC not found!"
msg_yes2        db 0x0D,0x0A,'$'
msg_setme       db "Invalid mode, please reset time and date",0x0D,0x0A,'$'
msg_lobatt      db "Battery is low, please replace battery",0x0D,0x0A,'$'
