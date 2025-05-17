; MS-DOS device drivers for DeviceMulti
; nasm -l multi.lst -o multi.sys multi.asm
                [warning -zeroing]
                [warning -label-orphan]
                cpu 186

; Device driver headers
kheader         dw c0header
                dw 0xFFFF
                dw 0x8008                       ; Char device, current CLOCK$
                dw strategy
                dw int_clock
                db "CLOCK$  "
dheader         dw c0header
                dw 0xFFFF
                dw 0x2000                       ; Block, non-IBM, non-removable
                dw strategy
                dw int_disk
                db 4,"MULTI  "                  ; Number of units
c0header        dw c1header
                dw 0xFFFF
                dw 0x8000                       ; Char device
                dw strategy
                dw int_char0
                db "MUL0    "
c1header        dw c2header
                dw 0xFFFF
                dw 0x8000                       ; Char device
                dw strategy
                dw int_char1
                db "MUL1    "
c2header        dw c3header
                dw 0xFFFF
                dw 0x8000                       ; Char device
                dw strategy
                dw int_char2
                db "MUL2    "
c3header        dw 0xFFFF
                dw 0xFFFF
                dw 0x8000                       ; Char device
                dw strategy
                dw int_char3
                db "MUL3    "

; Variables
request_off     resw 1                          ; Pointer to request packet
request_seg     resw 1
save_sp         resw 1                          ; Original stack pointer
save_ss         resw 1
io_addr         resw 1                          ; Base I/O address
retrycount      resb 1                          ; Count remaining disk retries
charnd_buf      resb 4                          ; Non-destructively-read bytes
charnd_flag     resb 4
                resb 256                        ; Local stack
newsp                                           ; Top of local stack

; Begin debug/trace stuff

setup_brkpt     push ds
                push bx
                xor bx,bx                       ; DS:BX => interrupt vectors
                mov ds,bx
                mov word [bx+0x4],int_brkpt     ; Install handler for trace
                mov [bx+0x6],cs
                mov word [bx+0xC],int_brkpt     ; Install handler for INT 3
                mov [bx+0xE],cs
                pop bx
                pop ds
                ret

int_brkpt       pusha
                mov dl,'['
                mov ah,2
                int 0x21

                mov bp,sp
                mov ax,[bp+18]                  ; Segment of return address
                call outhw
                mov dl,':'
                mov ah,2
                int 0x21
                mov ax,[bp+16]                  ; Offset of return address
                call outhw

                mov dl,']'
                mov ah,2
                int 0x21
                popa
int_iret        iret

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

; End debug/trace stuff

; Strategy entry point
strategy        mov cs:[request_off],bx
                mov cs:[request_seg],es
                ; call setup_brkpt                ; DEBUG
                retf

; Interrupt entry point for the clock driver
int_clock       push bx
                mov cs:[save_sp],sp
                mov cs:[save_ss],ss
                mov bx,cs
                mov ss,bx
                mov sp,newsp
                push ds
                mov ds,bx
                push di
                push es

                cld
                les di,[request_off]
                mov bl,es:[di+2]
                cmp bl,maxfunc_clock
                jbe .validfn
                mov bl,maxfunc_clock
.validfn        xor bh,bh
                shl bx,1
                call [bx+functbl_clock]
                or word es:[di+3],0x0100        ; Done

                pop es
                pop di
                pop ds
                mov ss,cs:[save_ss]
                mov sp,cs:[save_sp]
                pop bx
                retf

; Interrupt entry point for the disk driver
int_disk        push bx
                mov cs:[save_sp],sp
                mov cs:[save_ss],ss
                mov bx,cs
                mov ss,bx
                mov sp,newsp
                push ds
                mov ds,bx
                push di
                push es

                cld
                mov byte [retrycount],10
                les di,[request_off]
                mov bl,es:[di+2]
                cmp bl,maxfunc_disk
                jbe .validfn
                mov bl,maxfunc_disk
.validfn        xor bh,bh
                shl bx,1
                call [bx+functbl_disk]
                or word es:[di+3],0x0100        ; Done

                pop es
                pop di
                pop ds
                mov ss,cs:[save_ss]
                mov sp,cs:[save_sp]
                pop bx
                retf

; Interrupt entry point for the character driver
int_char0       push cx
                mov cl,0
                jmp int_char
int_char1       push cx
                mov cl,1
                jmp int_char
int_char2       push cx
                mov cl,2
                jmp int_char
int_char3       push cx
                mov cl,3
                jmp int_char

int_char        push bx
                mov cs:[save_sp],sp
                mov cs:[save_ss],ss
                mov bx,cs
                mov ss,bx
                mov sp,newsp
                push ds
                mov ds,bx
                push di
                push es

                cld
                les di,[request_off]
                mov es:[di+1],cl                ; Save our unit number
                mov bl,es:[di+2]
                cmp bl,maxfunc_char
                jbe .validfn
                mov bl,maxfunc_char
.validfn        xor bh,bh
                shl bx,1
                call [bx+functbl_char]
                or word es:[di+3],0x0100        ; Done

                pop es
                pop di
                pop ds
                mov ss,cs:[save_ss]
                mov sp,cs:[save_sp]
                pop bx
                pop cx
                retf

; Function table for the clock driver
functbl_clock   dw fn_clock_init                ; Initialise
                dw fn_badcmd                    ; Media check
                dw fn_badcmd                    ; Build BPB
                dw fn_badcmd                    ; IOCTL input
                dw fn_clock_input               ; Input (read)
                dw fn_badcmd                    ; Input, no wait
                dw fn_badcmd                    ; Input status
                dw fn_badcmd                    ; Input flush
                dw fn_nop                       ; Output (write)
                dw fn_nop                       ; Output, verify
                dw fn_badcmd                    ; Output status
maxfunc_clock   equ (($-functbl_clock)/2)-1

; Function table for the disk driver
functbl_disk    dw fn_disk_init                 ; Initialise
                dw fn_disk_media                ; Media check
                dw fn_disk_bpb                  ; Build BPB
                dw fn_badcmd                    ; IOCTL input
                dw fn_disk_input                ; Input (read)
                dw fn_badcmd                    ; Input, no wait
                dw fn_badcmd                    ; Input status
                dw fn_badcmd                    ; Input flush
                dw fn_disk_output               ; Output (write)
                dw fn_disk_output               ; Output, verify
                dw fn_badcmd                    ; Output status
maxfunc_disk    equ (($-functbl_disk)/2)-1

; Function table for the character driver
functbl_char    dw fn_char_init                 ; Initialise
                dw fn_badcmd                    ; Media check
                dw fn_badcmd                    ; Build BPB
                dw fn_badcmd                    ; IOCTL input
                dw fn_char_input                ; Input (read)
                dw fn_char_inputnw              ; Input, no wait
                dw fn_char_instat               ; Input status
                dw fn_char_inflush              ; Input flush
                dw fn_char_output               ; Output (write)
                dw fn_char_output               ; Output, verify
                dw fn_char_outstat              ; Output status
                dw fn_char_oflush               ; Output flush
                dw fn_badcmd                    ; IOCTL output
maxfunc_char    equ (($-functbl_char)/2)-1

; Do nothing, function not implemented
fn_badcmd       or word es:[di+3],0x8003
fn_nop          ret

; Read time/date from the RTC
fn_clock_input  push dx
                push cx
                push di
                push es

                les di,es:[di+0xE]              ; Destination in memory
                mov dx,[io_addr]
                add dx,0x10
                out dx,al                       ; Trigger a clock update
                mov cx,6                        ; Read the packet
                rep insb

                pop es
                pop di
                pop cx
                pop dx
                ret

; Media check
fn_disk_media   mov byte es:[di+0xE],1          ; Media has not changed
                ; int3
                ret

; Get BPB
fn_disk_bpb     mov word es:[di+0x12],1         ; Sector count
                mov word es:[di+0x14],0         ; Sector number
                call fn_disk_input              ; Read boot sector
                push dx
                mov dx,es:[di+0xE]              ; Point to BPB within buffer
                add dx,0xB
                mov es:[di+0x12],dx
                mov dx,es:[di+0x10]
                mov es:[di+0x14],dx
                pop dx
                ; int3
                ret

testbpb         dw 0x0200                       ; Bytes per logical sector
                db 0x04                         ; Logical sectors per cluster
                dw 0x0004                       ; Reserved logical sectors
                db 0x02                         ; Number of FATs
                dw 0x0200                       ; Root directory entries
                dw 0xA000                       ; Total logical sectors
                db 0xF8                         ; Media descriptor
                dw 0x0028                       ; Logical sectors per FAT
                dw 0x0020                       ; Physical sectors per track
                dw 0x0004                       ; Number of heads
                dw 0x0000                       ; Hidden sectors

bpbptrtbl       dw testbpb
                dw testbpb
                dw testbpb
                dw testbpb

; Input (Read)
fn_disk_input   mov bl,1
                jmp disk_rwcommon

; Output (write)
fn_disk_output  mov bl,2
                jmp disk_rwcommon

disk_rwcommon   pusha
                mov dx,[io_addr]                ; DX => unit/secsz/status
.waitlp         in al,dx                        ; Wait for not busy
                test al,0x10
                jnz .waitlp
                mov al,es:[di+0x1]              ; Select unit
                and al,0x3
                or al,0x8                       ; 512-byte sectors
                out dx,al

                add dx,0x02                     ; DX => sector count
                mov cx,es:[di+0x12]
                xor al,al
                out dx,al
                out dx,al
                mov al,ch
                out dx,al
                mov al,cl
                out dx,al
                shl cx,9                        ; CX = byte count

                add dx,0x02                     ; DX => LBA
                xor al,al
                out dx,al
                out dx,al
                mov al,es:[di+0x15]
                out dx,al
                mov al,es:[di+0x14]
                out dx,al

                add dx,0x02                     ; DX => command/error/status
                mov al,bl
                out dx,al                       ; Issue the command
.waitlp2        in al,dx                        ; Wait for DRQ or error
                test al,0x40
                jnz .err
                test al,0x80
                jz .waitlp2

                add dx,0x02                     ; DX => data
                test al,0x20                    ; Is the IN status bit set?
                jz .isoutput

                push es
                les di,es:[di+0xE]              ; Destination address
                rep insb                        ; Read some bytes
                pop es
                jmp .postdrq

.isoutput       push ds
                lds si,es:[di+0xE]              ; Source address
                rep outsb                       ; Write some bytes
                pop ds

.postdrq        sub dx,0x02                     ; DX => command/status
.waitlp3        in al,dx                        ; Wait for not busy
                test al,0x10
                jnz .waitlp3
                test al,0x40                    ; Check for error
                jz .exit

.err            les di,[request_off]
                and al,0xF                      ; Set error code and error bit
                or es:[di+3],al
                or word es:[di+3],0x8000
                mov word es:[di+0x12],0         ; Report 0 sectors transferred
.exit           popa                            ; ...or a successful exit
                ; int3
                ret

; Input (character device)
fn_char_input   push dx
                push cx
                push bx
                push ax
                push es
                push di

                mov cx,es:[di+0x12]             ; Number of bytes to read
                jcxz .skip
                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                mov bx,ax                       ; Save offset for ND buffer
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status
                les di,es:[di+0xE]              ; Destination address

                test byte [bx+charnd_flag],0x01 ; Buffer occupied?
                jz .lp                          ; No, go straight to hardware
                mov al,[bx+charnd_buf]          ; Yes, read it
                stosb
                and byte [bx+charnd_flag],~0x01 ; Mark the buffer empty
                jmp .lpend

.lp             in al,dx
                ; test al,0x38                    ; Errors?
                ; jnz .err
                test al,0x02                    ; RxRDY?
                jz .lp
                sub dx,0x02                     ; DX => data
                insb
                add dx,0x02                     ; DX => status
.lpend          loop .lp

.skip           pop di
                pop es
                pop ax
                pop bx
                pop cx
                pop dx
                ret

.err            les di,[request_off]
                or word es:[di+3],0x800B        ; Read fault
                sub es:[di+0x12],cx             ; Report bytes transferred
                ; int3
                jmp .skip

; Input, non-destructive / non-blocking
fn_char_inputnw push dx
                push bx
                push ax

                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                mov bx,ax                       ; Save offset for ND buffer
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status

                test byte [bx+charnd_flag],0x01 ; Buffer occupied?
                jz .empty                       ; No, go straight to hardware
                mov al,[bx+charnd_buf]          ; Yes, read it
                jmp .full

.empty          in al,dx
                ; test al,0x38                    ; Errors?
                ; jnz .err
                test al,0x80                    ; DSR?
                jz .busy
                test al,0x02                    ; RxRDY?
                jnz .notbusy
.busy           or word es:[di+3],0x0200        ; Busy
                jmp .exit
.notbusy        sub dx,0x02                     ; DX => data
                in al,dx
                mov [bx+charnd_buf],al          ; Non-destructive, save it
                or byte [bx+charnd_flag],0x01
.full           mov es:[di+0xD],al              ; Returned byte

.exit           pop ax
                pop bx
                pop dx
                ret

.err            or word es:[di+3],0x800B        ; Read fault
                ; int3
                jmp .exit

; Output (character device)
fn_char_output  push dx
                push cx
                push ax
                push ds
                push si

                mov cx,es:[di+0x12]             ; Number of bytes to write
                jcxz .skip
                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status
                lds si,es:[di+0xE]              ; Source address

.lp             in al,dx
                test al,0x01                    ; TxRDY?
                jz .lp
                sub dx,0x02                     ; DX => data
                outsb
                add dx,0x02                     ; DX => status
                loop .lp

.skip           pop si
                pop ds
                pop ax
                pop cx
                pop dx
                ret

; Output status
fn_char_outstat push dx
                push ax

                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status

                in al,dx
                test al,0x80                    ; DSR?
                jz .busy
                test al,0x01                    ; TxRDY?
                jnz .notbusy
.busy           or word es:[di+3],0x0200        ; Busy

.notbusy        pop ax
                pop dx
                ret

; Input status
fn_char_instat  push dx
                push bx
                push ax

                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                mov bx,ax                       ; Save offset for ND buffer
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status

                test byte [bx+charnd_flag],0x01 ; Buffer occupied?
                jnz .notbusy                    ; Yes, so we're not busy

                in al,dx
                test al,0x80                    ; DSR?
                jz .busy
                test al,0x02                    ; RxRDY?
                jnz .notbusy
.busy           or word es:[di+3],0x0200        ; Busy

.notbusy        pop ax
                pop bx
                pop dx
                ret

; Flush input buffer
fn_char_inflush push dx
                push bx
                push ax

                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                mov bx,ax                       ; Save offset for ND buffer
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status

                and byte [bx+charnd_flag],~0x01 ; Mark the buffer empty

.lp             in al,dx
                test al,0x02                    ; RxRDY?
                jz .empty
                sub dx,0x02                     ; DX => data
                in al,dx
                add dx,0x02                     ; DX => status
                jmp .lp

.empty          pop ax
                pop bx
                pop dx
                ret

; Flush output buffer
fn_char_oflush  push dx
                push ax

                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status

.lp             in al,dx
                test al,0x04                    ; TxEMPTY?
                jnz .empty
                test al,0x80                    ; DSR?
                jnz .lp

.empty          pop ax
                pop dx
                ret

; Code below here is only used during initialisation, then can be overwritten
init_break
fn_clock_init   lea dx,[msg_hello]
                mov ah,9
                int 0x21
                call probe
                test word [io_addr],0x780
                jnz .found

                lea dx,[msg_notfound]
                mov ah,9
                int 0x21
                mov word [kheader],0xFFFF       ; Skip the other inits
                mov word [kheader+4],0x8000     ; We won't be the new CLOCK$
                mov word es:[di+0xE],kheader    ; Everything can be overwritten
                mov es:[di+0x10],cs
                ret

.found          lea dx,[msg_found]
                mov ah,9
                int 0x21
                mov dx,[io_addr]                ; Get slot number from address
                shr dx,7
                and dl,7
                add dl,'0'
                mov ah,2
                int 0x21
                lea dx,[msg_crlf]
                mov ah,9
                int 0x21

                mov [kheader+2],cs              ; Pointers linking each driver
                mov [dheader+2],cs
                mov [c0header+2],cs
                mov [c1header+2],cs
                mov [c2header+2],cs
                ; mov word [dheader],0xFFFF       ; DEBUG skip other drivers
                ; mov word [dheader+2],0xFFFF

init_common     mov word es:[di+0xE],init_break
                mov es:[di+0x10],cs
                ; int3
                ret

fn_disk_init    mov al,[dheader+0xA]            ; Number of units
                mov es:[di+0xD],al
                mov word es:[di+0x12],bpbptrtbl ; BPB pointer table
                mov es:[di+0x14],cs

                lea dx,[msg_drvltr1]
                mov ah,9
                int 0x21
                mov dl,es:[di+0x16]             ; First available drive letter
                add dl,'A'
                mov ah,2
                int 0x21
                lea dx,[msg_drvltr2]
                mov ah,9
                int 0x21
                jmp init_common

; This bit is pointless, but let's pretend it's an actual 8251 for a minute
fn_char_init    push dx
                push bx
                push ax

                mov dx,[io_addr]
                mov al,es:[di+0x1]              ; Add unit number
                xor ah,ah
                mov bx,ax                       ; Save offset for ND buffer
                shl ax,2
                add dx,ax
                add dx,0x22                     ; DX => status

                and byte [bx+charnd_flag],~0x01 ; Mark the buffer empty

                mov al,0x4E                     ; 8N1, 16x- or internal reset?
                out dx,al
                out dx,al
                mov al,0x27                     ; Set DTR/RTS, enable Tx/Rx
                out dx,al

                pop ax
                pop bx
                pop dx
                jmp init_common

msg_hello       db 0x0D,0x0A,"Multi-I/O Device Emulator for RM Nimbus PC-186"
                db 0x0D,0x0A,"(c)2025 David Knoll "
                db "[https://github.com/davidknoll/rmnimbus]",0x0D,0x0A,"$"
msg_found       db "Multi-I/O device found at card select $"
msg_notfound    db "Multi-I/O device not found!"
msg_crlf        db 0x0D,0x0A,"$"
msg_drvltr1     db "Emulated drives will begin at $"
msg_drvltr2     db ":",0x0D,0x0A,"$"

; Find the base address of the multi-I/O device, if any
; Entry: None
; Exit:  [io_addr] = base I/O address, or zero if not found in any slot
probe           push dx
                push cx
                push ax
                mov cx,0x400                    ; Start at slot 0

.probelp        mov dx,cx
                add dx,0x7C                     ; Read 1st magic number
                in al,dx
                cmp al,0xC1
                jnz .nothere
                add dx,0x02                     ; Read 2nd magic number
                in al,dx
                cmp al,0x86
                jnz .nothere

.end            mov cs:[io_addr],cx             ; Save found base address
                pop ax
                pop cx
                pop dx
                ret

.nothere        add cx,0x80                     ; Next slot
                cmp cx,0x680                    ; Past the end?
                jc .probelp
                xor cx,cx                       ; Not found at all
                jmp .end

the_end
