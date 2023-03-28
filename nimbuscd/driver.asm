	page	60,132
	title	Assembler Device Driver

;****************************************************************
;*	This is an assembler device driver front-end for C	*
;*								*
;*	Each command will be processed by a C routine		*
;****************************************************************

;****************************************************************
;*	INSTRUCTING THE ASSEMBLER				*
;*								*
;*	We need the _DATA first in order to have		*
;*	the Device Header at the beginning of the file.		*
;*	Therefore, we use the group command to order the	*
;*	segments with _DATA first.				*
;*								*
;****************************************************************

DGROUP	group	_DATA, CONST, _BSS, _TEXT	;linker order

_DATA	segment	word public 'DATA'
	assume	ds:DGROUP

;****************************************************************
;*	DEVICE HEADER REQUIRED BY DOS				*
;****************************************************************

		public	_deviceheader		;C accessible
		org	0			;relative location 0

_deviceheader	label	word			;device header label
next_dev	dd	-1			;no next device driver
attribute	dw	0C800h			;char, ioctl, removable
strategy	dw	DGROUP:_strategy	;strategy address
interrupt	dw	DGROUP:_interrupt	;interrupt address
dev_name	db	'NIMBUSCD'		;name of device

;****************************************************************
;*	CD-ROM DEVICE HEADER					*
;****************************************************************

reserved	dw	0			;reserved
drive		db	0			;drive letter
units		db	1			;number of CD ROM units

;****************************************************************
;*	REQUEST HEADER						*
;****************************************************************

		public	_rhptr			;make Request Header
						;pointer C accessible
_rhptr		equ	$			;Request Header
rh_ofs		dw	?			; offset address
rh_seg		dw	?			; segment address

;****************************************************************
;*	NEW STACK DEFINITIONS					*
;****************************************************************

stack_ptr	dw	?			;old stack pointer
stack_seg	dw	?			;old stack segment

newstack	db	100h dup (?)		;new stack defined here
newstacktop	label	word			;top of new stack

_DATA	ends

CONST	segment	word public 'CONST'
	assume	ds:DGROUP
CONST	ends

_BSS	segment	word public 'BSS'
	assume	ds:DGROUP
_BSS	ends

_TEXT	segment	word public 'CODE'
	assume	cs:DGROUP,ds:DGROUP

;****************************************************************
;*	THE STRATEGY PROCEDURE					*
;****************************************************************

		public	_strategy	;externally accessible

_strategy	proc	far		;also C referenceable
		mov	cs:rh_seg,es	;save segment address
		mov	cs:rh_ofs,bx	;save offset address
		ret			;return to DOS
_strategy	endp

;****************************************************************
;*	THE INTERRUPT PROCEDURE					*
;****************************************************************

		public	_interrupt	;externally accessible

_interrupt	proc	far		;also C referenceable

;save machine state on entry
		cld
		push	ds
		push	es
		push	ax
		push	bx
		push	cx
		push	dx
		push	di
		push	si

;switch to new and larger stack for C use

		cli			;turn interrupts off
		mov	cs:stack_ptr,sp	;save old stack ptr
		mov	cs:stack_seg,ss	;save old stack seg reg
		mov	ax,cs		;get current segment
		mov	ss,ax		;set stack segment
		mov	sp,newstacktop	;set stack pointer
		sti			;re-enable interrupts

		mov	ax,cs:rh_seg	;restore ES saved by
		mov	es,ax		; STRATEGY call
		mov	bx,cs:rh_ofs	;same for BX

;jump to appropriate routine to process command

		mov	al,es:[bx]+2	;get req hdr command
		cmp	al,128		;calling a CD-ROM function?
		jc	notcd
		rol	al,1
		lea	di,cmdtab2	;if so, use the second table
		jmp	wascd

notcd:		rol	al,1		;x2 for word table index
		lea	di,cmdtab	;command table address
wascd:		mov	ah,0		;clear hi order
		add	di,ax		;add index to table start
		call	word ptr[di]	;call C routine

;note that the return from the C routine does not require
;us to extract any returned arguments nor function value

;switch back to old stack
		cli			;turn interrupts off
		mov	ss,cs:stack_seg	;restore old SS
		mov	sp,cs:stack_ptr	;restore old SP
		sti			;re-enable interrupts

;restore registers and exit back to DOS

		pop	si
		pop	di
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		pop	es
		pop	ds
		ret			;return to DOS

;the following are the C routines to process each
;device driver command. Note that the leading underscore
;is required for C access.

	EXTRN	_init:near
;	EXTRN	_mediacheck:near
;	EXTRN	_getbpb:near
	EXTRN	_ioctlinput:near
;	EXTRN	_input:near
;	EXTRN	_ndinput:near
;	EXTRN	_inputstatus:near
	EXTRN	_inputflush:near
;	EXTRN	_output:near
;	EXTRN	_outputverify:near
;	EXTRN	_outputstatus:near
;	EXTRN	_outputflush:near
	EXTRN	_ioctloutput:near
	EXTRN	_deviceopen:near
	EXTRN	_deviceclose:near
;	EXTRN	_removeable:near
;	EXTRN	_outputbusy:near
	EXTRN	_badcommand:near
;	EXTRN	_genericioctl:near
;	EXTRN	_getdevice:near
;	EXTRN	_setdevice:near
;	EXTRN	_ioctlquery:near

	EXTRN	_readlong:near
	EXTRN	_readlongprefetch:near
	EXTRN	_seek:near
	EXTRN	_playaudio:near
	EXTRN	_stopaudio:near
	EXTRN	_writelong:near
	EXTRN	_writelongverify:near
	EXTRN	_resumeaudio:near

;CMDTAB is the command table that contains the word address
;for each command. The request header will contain the
;command desired. The INTERRUPT routine will jump through an
;address corresponding to the requested command to get to
;the appropriate command processing routine.

CMDTAB	label	word			;* = char devices only
	dw	DGROUP:_init		; initialization
	dw	DGROUP:_badcommand	; media check (block)
	dw	DGROUP:_badcommand	; build bpb
	dw	DGROUP:_ioctlinput	; ioctl in
	dw	DGROUP:_badcommand	; input (read)
	dw	DGROUP:_badcommand	;*nd input no wait
	dw	DGROUP:_badcommand	;*input status
	dw	DGROUP:_inputflush	;*input flush
	dw	DGROUP:_badcommand 	; output (write)
	dw	DGROUP:_badcommand	; output (write) verify
	dw	DGROUP:_badcommand	;*output status
	dw	DGROUP:_badcommand	;*output flush
	dw	DGROUP:_ioctloutput	; ioctl output
	dw	DGROUP:_deviceopen	; device open
	dw	DGROUP:_deviceclose	; device close
	dw	DGROUP:_badcommand	; removeable media
	dw	DGROUP:_badcommand	; output til busy
	dw	DGROUP:_badcommand	; undefined
	dw	DGROUP:_badcommand	; undefined
	dw	DGROUP:_badcommand	; generic ioctl
	dw	DGROUP:_badcommand	; undefined
	dw	DGROUP:_badcommand	; undefined
	dw	DGROUP:_badcommand	; undefined
	dw	DGROUP:_badcommand	; get logical device
	dw	DGROUP:_badcommand	; set logical device
	dw	DGROUP:_badcommand	; ioctl query

CMDTAB2	label	word				; CD-ROM devices only
	dw	DGROUP:_readlong		; read long
	dw	DGROUP:_badcommand		; reserved
	dw	DGROUP:_readlongprefetch	; read long prefetch
	dw	DGROUP:_seek			; seek
	dw	DGROUP:_playaudio		; play audio
	dw	DGROUP:_stopaudio		; stop audio
	dw	DGROUP:_writelong		; write long
	dw	DGROUP:_writelongverify		; write long verify
	dw	DGROUP:_resumeaudio		; resume audio

_interrupt	endp			;end of _interrupt

;****************************************************************
;*	END OF PROGRAM						*
;****************************************************************

;that's all folks
_TEXT	ends
	end
