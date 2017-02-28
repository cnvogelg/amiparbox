; --- proto signals ---
; POUT = /CLK  (Amiga out)
; BUSY = /RAK  (Amiga in)
; SEL  = ?

        include         "pario.i"
        include         "proto.i"

        xdef            _proto_low_no_value
        xdef            _proto_low_write_word
        xdef            _proto_low_read_word
        xdef            _proto_low_write_block
        xdef            _proto_low_read_block

; ----- macros --------------------------------------------------------------

; --- setup_port_regs ---
; in:  a0 = port_ptr
; out: d2 = rak bit
;      d3 = clk bit
;      d4 = old ctrl val
;      a3 = data port
;      a4 = data ddr
;      a5 = ctrl port
setup_port_regs  MACRO
        moveq   #0,d2
        moveq   #0,d3
        move.b  PO_BUSY_BIT(a0),d2
        move.b  PO_POUT_BIT(a0),d3
        move.l  PO_DATA_PORT(a0),a3
        move.l  PO_DATA_DDR(a0),a4
        move.l  PO_CTRL_PORT(a0),a5
        ; read current ctrl val
        move.b  (a5),d4
        ENDM


; --- check_rak_hi ---
; check if RAK is high otherwise rts with error
; \1 end label
check_rak_hi  MACRO
        btst    d2,(a5)
        bne.s   \@
        moveq   #RET_RAK_INVALID,d0
        bra     \1
\@:
        ENDM


; --- check_rak_lo ---
; check if RAK is high otherwise rts with error
; \1 end label
check_rak_lo  MACRO
        btst    d2,(a5)
        beq.s   \@
        moveq   #RET_SLAVE_ERROR,d0
        bra     \1
\@:
        ENDM


; --- clk_lo ---
; set CLK signal to low
clk_lo  MACRO
        bclr    d3,d4
        move.b  d4,(a5)
        ENDM


; --- clk_hi ---
; set CLK signal to high
clk_hi  MACRO
        bset    d3,d4
        move.b  d4,(a5)
        ENDM


; --- wait_rak_lo ---
; wait for RAK to become low or if timeout triggers
; \1 = jump label on timeout
wait_rak_lo  MACRO
        ; check RAK level
\@1:    btst    d2,(a5)
        beq.s   \@2
        ; check for timeout
        tst.b   (a1)
        beq.s   \@1
        ; error
        moveq   #RET_TIMEOUT,d0
        bra     \1
\@2:
        ENDM


; --- wait_rak_hi ---
; wait for RAK to become high or if timeout triggers
; \1 = jump label on timeout
wait_rak_hi  MACRO
        ; check RAK level
\@1:    btst    d2,(a5)
        bne.s   \@2
        ; check for timeout
        tst.b   (a1)
        beq.s   \@1
        ; error
        moveq   #RET_TIMEOUT,d0
        bra     \1
\@2:
        ENDM


; --- ddr_in ---
; set data direction to input
ddr_in  MACRO
        sf.b    (a4)
        ENDM


; --- ddr_out ---
; set data direction to output
ddr_out  MACRO
        st.b    (a4)
        ENDM


; --- set_cmd_idle ---
; set command to idle (0)
set_cmd_idle  MACRO
        sf.b    (a3)
        ENDM


; --- set_cmd ---
; set a command byte to data port
; \1 = cmd constant
set_cmd  MACRO
        move.b  \1,(a3)
        ENDM


; --- set_data ---
; set data port
; \1 = value to set
set_data MACRO
        move.b  \1,(a3)
        ENDM


; --- get_data ---
; get data from port
; \1 = store value
get_data MACRO
        move.b  (a3),\1
        ENDM


; ----- functions -----------------------------------------------------------

; --- proto_low_no_value ---
; a simple command that does not transfer any value
;
;   in:
;       a0      struct pario_port *port
;       a1      volatile UBYTE *timeout_flag
;       d0      CMD_PING constant
;   out:
;       d0      return code
_proto_low_no_value:
        movem.l d2-d7/a2-a6,-(sp)

        ; setup regs with port values and read old ctrl value
        setup_port_regs

        ; -- sync with slave
        ; check RAK to be high or abort
        check_rak_hi    plp_end
        ; set cmd to data port
        set_cmd         d0
        ; set CLK to low (active) to trigger command at slave
        clk_lo
        ; busy wait with timeout for RAK to go low
        ; (we wait for the slave to react/sync)
        wait_rak_lo     plp_abort

        ; now we are in sync with slave
        ; we are now in work phase
        ; but ping does nothing here

        ; -- final sync
        ; now raise CLK again
        clk_hi
        ; expect slave to raise rak, too
        check_rak_hi    plp_end

        ; ok
        moveq   #RET_OK,d0
plp_end:
        ; restore cmd
        set_cmd_idle

        movem.l (sp)+,d2-d7/a2-a6
        rts
plp_abort:
        ; ensure CLK is hi
        clk_hi
        bra.s   plp_end


; --- proto_low_write_word ---
; in:  a0 = port ptr
;      a1 = timeout byte ptr
;      a2 = ptr to byte
;      d0 = cmd byte
; out: d0 = result
_proto_low_write_word:
        movem.l d2-d7/a2-a6,-(sp)
        setup_port_regs

        ; sync with slave
        check_rak_hi    plrw_end
        set_cmd         d0
        clk_lo
        wait_rak_lo     plrw_abort

        ; -- first byte
        ; setup test value on data lines
        set_data        (a2)+
        ; signal to slave to read the value
        clk_hi

        ; -- second byte
        set_data        (a2)+
        clk_lo

        ; final sync
        clk_hi
        ; wait for slave done
        check_rak_hi    plrw_end

        ; ok
        moveq   #RET_OK,d0
plrw_end:
        set_cmd_idle
        movem.l (sp)+,d2-d7/a2-a6
        rts
plrw_abort:
        ; ensure CLK is hi
        clk_hi
        bra.s    plrw_end


; --- proto_low_read_word ---
; in:  a0 = port ptr
;      a1 = timeout byte ptr
;      a2 = ptr to test byte
;      d0 = cmd byte
; out: d0 = result
_proto_low_read_word:
        movem.l d2-d7/a2-a6,-(sp)
        setup_port_regs

        ; sync with slave
        check_rak_hi    plrr_end
        set_cmd         d0
        clk_lo
        wait_rak_lo     plrr_abort

        ; switch: port read
        ddr_in
        clk_hi

        ; first byte
        ; signal read to slave
        clk_lo
        ; read value from data port
        get_data        (a2)+

        ; second bytes
        clk_hi
        get_data        (a2)+

        ; switch: port write
        clk_lo
        ddr_out

        ; final sync
        clk_hi
        check_rak_hi    plrr_end

        ; ok
        moveq   #RET_OK,d0
plrr_end:
        set_cmd_idle
        movem.l (sp)+,d2-d7/a2-a6
        rts
plrr_abort:
        ; ensure CLK is hi
        clk_hi
        bra.s    plrr_end


; --- proto_low_write_block ---
; in:  a0 = port ptr
;      a1 = timeout byte ptr
;      a2 = ptr to ptroto_msg
;      d0 = cmd
; out: d0 = result
_proto_low_write_block:
        movem.l d2-d7/a2-a6,-(sp)
        setup_port_regs

        ; sync with slave
        check_rak_hi    plrw_end
        set_cmd         d0
        clk_lo

        ; prepare size while waiting for slave
        moveq           #0,d1
        move.w          4(a2),d1
        move.w          d1,d5
        lsr.w           #8,d5

        ; wait for slave sync
        wait_rak_lo     plrw_abort

        ; send size
        set_data        d5 ; hi byte
        clk_hi
        set_data        d1 ; lo byte
        clk_lo

        ; empty message?
        tst.w           d1
        beq.s           plmw_done
        subq.w          #1,d1 ; for dbra

        ; get buffer
        move.l          (a2),a0

        ; data block loop
plmw_loop:
        ; odd byte
        set_data        (a0)+
        clk_hi
        ; even byte
        set_data        (a0)+
        clk_lo

        dbra            d1,plmw_loop

plmw_done:
        ; final sync
        clk_hi
        check_rak_hi    plmw_end

        ; ok
        moveq   #RET_OK,d0
plmw_end:
        set_cmd_idle
        movem.l (sp)+,d2-d7/a2-a6
        rts
plmw_abort:
        ; ensure CLK is hi
        clk_hi
        bra.s    plmw_end


; --- proto_low_read_block ---
; in:  a0 = port ptr
;      a1 = timeout byte ptr
;      a2 = ptr to proto_msg
;      d0 = cmd byte
; out: d0 = result
_proto_low_read_block:
        movem.l d2-d7/a2-a6,-(sp)
        setup_port_regs

        ; sync with slave
        check_rak_hi    plmr_end
        set_cmd         d0
        clk_lo

        ; prepare size regs
        moveq           #0,d5
        moveq           #0,d6

        ; final slave sync
        wait_rak_lo     plmr_abort

        ; switch: port read
        ddr_in
        clk_hi

        ; read size
        clk_lo
        get_data        d5 ; hi
        clk_hi
        get_data        d6 ; lo

        ; combine size lo/hi -> d5
        lsl.w           #8,d5
        or.w            d6,d5
        ; store real size: proto_msg->num_words
        move.w          d5,4(a2)
        ; check size
        tst.w           d5 ; empty message
        beq.s           plmr_done_ok
        cmp.w           6(a2),d5 ; proto_msg->max_words
        bls.s           plmr_ok

        ; size invalid - run a fake loop a do not store data
        moveq           #RET_MSG_TOO_LARGE,d0
        subq.w          #1,d5
plmr_fake_loop:
        clk_lo
        get_data        d1
        clk_hi
        get_data        d1
        dbra            d5,plmr_fake_loop
        bra.s           plmr_done

plmr_ok:
        ; size valid - run a copy loop and store data
        subq.w          #1,d5
        move.l          (a2),a0 ; data buffer
        ; data copy loop
plmr_copy_loop:
        ; odd byte
        clk_lo
        get_data        (a0)+
        ; even byte
        clk_hi
        get_data        (a0)+
        dbra            d5,plmr_copy_loop
plmr_done_ok:
        ; ok
        moveq   #RET_OK,d0

plmr_done:
        ; switch: port write
        clk_lo
        ddr_out

        ; final sync
        clk_hi
        check_rak_hi    plmr_end
plmr_end:
        set_cmd_idle
        movem.l (sp)+,d2-d7/a2-a6
        rts
plmr_abort:
        ; ensure CLK is hi
        clk_hi
        bra.s           plmr_end

