; Real-mode portions used by the Build multiplayer module.
; Source reconstruction of the serial IRQ template.
; Linked as a temporary COM by link_probe.py to initialize rmbuffer in MULTI.C.

.386

MULTI_RM SEGMENT BYTE PUBLIC USE16 'CODE'
    ASSUME CS:MULTI_RM
    PUBLIC _rmbuffer

RING_SIZE EQU 4096
SERIAL_TEMPLATE_SIZE EQU 384
INPUT_RING EQU SERIAL_TEMPLATE_SIZE
OUTPUT_RING EQU SERIAL_TEMPLATE_SIZE + RING_SIZE
ESCAPE_BYTE EQU 081h
END_PACKET EQU 07eh

template_start LABEL NEAR
_rmbuffer LABEL BYTE
ring_read       DW 0
ring_write      DW 0
output_read     DW 0
output_write    DW 0
uart_port       DW 0
uart_fifo_size  DB 0
pending_reply   DB 0
incoming_seq    DB 0
last_packet     DD 0
total_clock     DD 0

serial_irq:
    push ax
    push bx
    push ecx
    push dx

poll_interrupt:
    mov dx, cs:[uart_port - _rmbuffer]
    add dx, 2
    in al, dx
    and al, 7
    mov ah, al
    cmp ah, 2
    je near ptr transmit
    cmp ah, 4
    je near ptr receive
    jmp near ptr poll_again

transmit:
    mov dx, cs:[uart_port - _rmbuffer]
    mov cl, cs:[uart_fifo_size - _rmbuffer]
transmit_next:
    sub cl, 1
    js near ptr poll_again
    cmp byte ptr cs:[pending_reply - _rmbuffer], 1
    jl near ptr send_buffered
    je near ptr send_sequence
    mov al, ESCAPE_BYTE
    jmp short send_reply
    nop
send_sequence:
    mov al, cs:[incoming_seq - _rmbuffer]
send_reply:
    out dx, al
    dec byte ptr cs:[pending_reply - _rmbuffer]
    jmp short transmit_next

send_buffered:
    mov bx, cs:[output_read - _rmbuffer]
    cmp bx, cs:[output_write - _rmbuffer]
    je near ptr poll_again
    mov al, cs:[bx + OUTPUT_RING]
    out dx, al
    inc bx
    and bx, RING_SIZE - 1
    mov cs:[output_read - _rmbuffer], bx
    jmp short transmit_next

receive:
    mov dx, cs:[uart_port - _rmbuffer]
    in al, dx
    mov bx, cs:[ring_write - _rmbuffer]
    mov cs:[bx + INPUT_RING], al
    inc bx
    and bx, RING_SIZE - 1
    mov cs:[ring_write - _rmbuffer], bx
    cmp al, END_PACKET
    jne near ptr check_fifo
    mov ecx, cs:[total_clock - _rmbuffer]
    mov cs:[last_packet - _rmbuffer], ecx
check_fifo:
    cmp byte ptr cs:[uart_fifo_size - _rmbuffer], 16
    jne near ptr poll_again
    add dx, 5
    in al, dx
    test al, 1
    jne short receive

poll_again:
    test ah, 1
    je near ptr poll_interrupt
    mov al, 20h
    out 20h, al
    pop dx
    pop ecx
    pop bx
    pop ax
    iret
    DB (SERIAL_TEMPLATE_SIZE - ($ - _rmbuffer)) DUP (0)
MULTI_RM ENDS

END template_start
