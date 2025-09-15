;/*
; * File      : context_gcc.S
; * This file is part of RT-Thread RTOS
; * COPYRIGHT (C) 2018, RT-Thread Development Team
; *
; *  This program is free software; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License along
; *  with this program; if not, write to the Free Software Foundation, Inc.,
; *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
; *
; * Change Logs:
; * Date           Author       Notes
; * 2017-07-16     zhangjun     for hifive1
; * 2018-05-29     tanek        optimize  rt_hw_interrupt_*
; * 2018-05-29     tanek        add mie register to context
; */

#include "rtconfig.h"

#ifdef ARCH_RISCV_FPU_S
  #define SWITCH_STACK_SIZE ((72)*4)
#else
  #define SWITCH_STACK_SIZE ((38)*4)
#endif

/*
 * rt_base_t rt_hw_interrupt_disable(void);
 */
    .globl rt_hw_interrupt_disable
rt_hw_interrupt_disable:
    csrrci a0, mstatus, 8
    ret

/*
 * void rt_hw_interrupt_enable(rt_base_t level);
 */
    .globl rt_hw_interrupt_enable
rt_hw_interrupt_enable:
    csrw mstatus, a0
    ret

/*
 * void rt_hw_context_switch(rt_uint32 from, rt_uint32 to);
 * a0 --> from
 * a1 --> to
 */
    .globl rt_hw_context_switch
rt_hw_context_switch:

    /* saved from thread context
     *     x1/ra       -> sp(0)
     *     x1/ra       -> sp(1)
     *     mstatus.mie -> sp(2)
     *     x(i)        -> sp(i-4)
     */
    addi  sp,  sp, -SWITCH_STACK_SIZE
    sw sp,  (a0)

    sw x1,   0 * 4(sp)
    sw x1,   1 * 4(sp)

    csrr a0, mstatus
    andi a0, a0, 8
    beqz a0, save_mpie
    li   a0, 0x80
save_mpie:
    sw a0,   2 * 4(sp)

    sw x4,   4 * 4(sp)
    sw x5,   5 * 4(sp)
    sw x6,   6 * 4(sp)
    sw x7,   7 * 4(sp)
    sw x8,   8 * 4(sp)
    sw x9,   9 * 4(sp)
    sw x10, 10 * 4(sp)
    sw x11, 11 * 4(sp)
    sw x12, 12 * 4(sp)
    sw x13, 13 * 4(sp)
    sw x14, 14 * 4(sp)
    sw x15, 15 * 4(sp)
    sw x16, 16 * 4(sp)
    sw x17, 17 * 4(sp)
    sw x18, 18 * 4(sp)
    sw x19, 19 * 4(sp)
    sw x20, 20 * 4(sp)
    sw x21, 21 * 4(sp)
    sw x22, 22 * 4(sp)
    sw x23, 23 * 4(sp)
    sw x24, 24 * 4(sp)
    sw x25, 25 * 4(sp)
    sw x26, 26 * 4(sp)
    sw x27, 27 * 4(sp)
    sw x28, 28 * 4(sp)
    sw x29, 29 * 4(sp)
    sw x30, 30 * 4(sp)
    sw x31, 31 * 4(sp)

    // hwloop
    csrr t0, 0x7B0
    csrr t1, 0x7B1
    csrr t2, 0x7B2
    sw t0, 32*4(sp)  // lpstart[0]
    sw t1, 33*4(sp)  // lpend[0]
    sw t2, 34*4(sp)  // lpcount[0]
    csrr t0, 0x7B4
    csrr t1, 0x7B5
    csrr t2, 0x7B6
    sw t0, 35*4(sp)  // lpstart[1]
    sw t1, 36*4(sp)  // lpend[1]
    sw t2, 37*4(sp)  // lpcount[1]

#ifdef ARCH_RISCV_FPU_S
  csrr t0, 0x03  // fcsr
  csrr t1, 0x06  // fprec
  sw t0, 38*4(sp)  
  sw t1, 39*4(sp) 

  // fp reg
  fsw ft0,40*4(sp)  
  fsw ft1,41*4(sp)  
  fsw ft2,42*4(sp)  
  fsw ft3,43*4(sp)  
  fsw ft4,44*4(sp)  
  fsw ft5,45*4(sp)  
  fsw ft6,46*4(sp)  
  fsw ft7,47*4(sp)  
  fsw fs0,48*4(sp)  
  fsw fs1,49*4(sp)  
  fsw fa0,50*4(sp)  
  fsw fa1,51*4(sp)  
  fsw fa2,52*4(sp)  
  fsw fa3,53*4(sp)  
  fsw fa4,54*4(sp)  
  fsw fa5,55*4(sp)  
  fsw fa6,56*4(sp)  
  fsw fa7,57*4(sp)  
  fsw fs2,58*4(sp)  
  fsw fs3,59*4(sp)  
  fsw fs4,60*4(sp)  
  fsw fs5,61*4(sp)  
  fsw fs6,62*4(sp)  
  fsw fs7,63*4(sp)  
  fsw fs8,64*4(sp)  
  fsw fs9,65*4(sp)  
  fsw fs10,66*4(sp)  
  fsw fs11,67*4(sp)  
  fsw ft8,68*4(sp)  
  fsw ft9,69*4(sp)  
  fsw ft10,70*4(sp)  
  fsw ft11,71*4(sp)   

#endif

    /* restore to thread context
     * sp(0) -> epc;
     * sp(1) -> ra;
     * sp(i) -> x(i+2)
     */
    lw sp,  (a1)

    // hwloop
    lw t0, 32*4(sp)  // lpstart[0]
    lw t1, 33*4(sp)  // lpend[0]
    lw t2, 34*4(sp)  // lpcount[0]
    csrw 0x7B0,t0
    csrw 0x7B1,t1
    csrw 0x7B2,t2
    lw t0, 35*4(sp)  // lpstart[1]
    lw t1, 36*4(sp)  // lpend[1]
    lw t2, 37*4(sp)  // lpcount[1]
    csrw 0x7B4,t0
    csrw 0x7B5,t1
    csrw 0x7B6,t2

#ifdef ARCH_RISCV_FPU_S

    // fp reg

    lw t0, 38*4(sp)  // fcsr
    lw t1, 39*4(sp)  // fprec
    csrw 0x3,t0      // 
    csrw 0x6,t1      // 

    flw ft0,40*4(sp)  
    flw ft1,41*4(sp)  
    flw ft2,42*4(sp)  
    flw ft3,43*4(sp)  
    flw ft4,44*4(sp)  
    flw ft5,45*4(sp)  
    flw ft6,46*4(sp)  
    flw ft7,47*4(sp)  
    flw fs0,48*4(sp)  
    flw fs1,49*4(sp)  
    flw fa0,50*4(sp)  
    flw fa1,51*4(sp)  
    flw fa2,52*4(sp)  
    flw fa3,53*4(sp)  
    flw fa4,54*4(sp)  
    flw fa5,55*4(sp)  
    flw fa6,56*4(sp)  
    flw fa7,57*4(sp)  
    flw fs2,58*4(sp)  
    flw fs3,59*4(sp)  
    flw fs4,60*4(sp)  
    flw fs5,61*4(sp)  
    flw fs6,62*4(sp)  
    flw fs7,63*4(sp)  
    flw fs8,64*4(sp)  
    flw fs9,65*4(sp)  
    flw fs10,66*4(sp)  
    flw fs11,67*4(sp)  
    flw ft8,68*4(sp)  
    flw ft9,69*4(sp)  
    flw ft10,70*4(sp)  
    flw ft11,71*4(sp) 
#endif

    /* resw ra to mepc */
    lw a1,   0 * 4(sp)
    csrw mepc, a1
    lw x1,   1 * 4(sp)

    /* force to machin mode(MPP=11) */
    li a1, 0x00001800;
    csrs mstatus, a1
    lw a1,   2 * 4(sp)
    csrs mstatus, a1

    lw x4,   4 * 4(sp)
    lw x5,   5 * 4(sp)
    lw x6,   6 * 4(sp)
    lw x7,   7 * 4(sp)
    lw x8,   8 * 4(sp)
    lw x9,   9 * 4(sp)
    lw x10, 10 * 4(sp)
    lw x11, 11 * 4(sp)
    lw x12, 12 * 4(sp)
    lw x13, 13 * 4(sp)
    lw x14, 14 * 4(sp)
    lw x15, 15 * 4(sp)
    lw x16, 16 * 4(sp)
    lw x17, 17 * 4(sp)
    lw x18, 18 * 4(sp)
    lw x19, 19 * 4(sp)
    lw x20, 20 * 4(sp)
    lw x21, 21 * 4(sp)
    lw x22, 22 * 4(sp)
    lw x23, 23 * 4(sp)
    lw x24, 24 * 4(sp)
    lw x25, 25 * 4(sp)
    lw x26, 26 * 4(sp)
    lw x27, 27 * 4(sp)
    lw x28, 28 * 4(sp)
    lw x29, 29 * 4(sp)
    lw x30, 30 * 4(sp)
    lw x31, 31 * 4(sp)

    addi sp,  sp, SWITCH_STACK_SIZE
    mret

/*
 * void rt_hw_context_switch_to(rt_uint32 to);
 * a0 --> to
 */
    .globl rt_hw_context_switch_to
rt_hw_context_switch_to:
    lw sp, (a0)

    // hwloop
    lw t0, 32*4(sp)  // lpstart[0]
    lw t1, 33*4(sp)  // lpend[0]
    lw t2, 34*4(sp)  // lpcount[0]
    csrw 0x7B0,t0
    csrw 0x7B1,t1
    csrw 0x7B2,t2
    lw t0, 35*4(sp)  // lpstart[1]
    lw t1, 36*4(sp)  // lpend[1]
    lw t2, 37*4(sp)  // lpcount[1]
    csrw 0x7B4,t0
    csrw 0x7B5,t1
    csrw 0x7B6,t2

    /* load epc from stack */
    lw a0,   0 * 4(sp)
    csrw mepc, a0
    lw x1,   1 * 4(sp)
    /* load mstatus from stack */
    lw a0,   2 * 4(sp)
    csrw mstatus, a0
    lw x4,   4 * 4(sp)
    lw x5,   5 * 4(sp)
    lw x6,   6 * 4(sp)
    lw x7,   7 * 4(sp)
    lw x8,   8 * 4(sp)
    lw x9,   9 * 4(sp)
    lw x10, 10 * 4(sp)
    lw x11, 11 * 4(sp)
    lw x12, 12 * 4(sp)
    lw x13, 13 * 4(sp)
    lw x14, 14 * 4(sp)
    lw x15, 15 * 4(sp)
    lw x16, 16 * 4(sp)
    lw x17, 17 * 4(sp)
    lw x18, 18 * 4(sp)
    lw x19, 19 * 4(sp)
    lw x20, 20 * 4(sp)
    lw x21, 21 * 4(sp)
    lw x22, 22 * 4(sp)
    lw x23, 23 * 4(sp)
    lw x24, 24 * 4(sp)
    lw x25, 25 * 4(sp)
    lw x26, 26 * 4(sp)
    lw x27, 27 * 4(sp)
    lw x28, 28 * 4(sp)
    lw x29, 29 * 4(sp)
    lw x30, 30 * 4(sp)
    lw x31, 31 * 4(sp)

#ifdef ARCH_RISCV_FPU_S

    // fp reg

    lw t0, 38*4(sp)  // fcsr
    lw t1, 39*4(sp)  // fprec
    csrw 0x3,t0      
    csrw 0x6,t1      

    flw ft0,40*4(sp)  
    flw ft1,41*4(sp)  
    flw ft2,42*4(sp)  
    flw ft3,43*4(sp)  
    flw ft4,44*4(sp)  
    flw ft5,45*4(sp)  
    flw ft6,46*4(sp)  
    flw ft7,47*4(sp)  
    flw fs0,48*4(sp)  
    flw fs1,49*4(sp)  
    flw fa0,50*4(sp)  
    flw fa1,51*4(sp)  
    flw fa2,52*4(sp)  
    flw fa3,53*4(sp)  
    flw fa4,54*4(sp)  
    flw fa5,55*4(sp)  
    flw fa6,56*4(sp)  
    flw fa7,57*4(sp)  
    flw fs2,58*4(sp)  
    flw fs3,59*4(sp)  
    flw fs4,60*4(sp)  
    flw fs5,61*4(sp)  
    flw fs6,62*4(sp)  
    flw fs7,63*4(sp)  
    flw fs8,64*4(sp)  
    flw fs9,65*4(sp)  
    flw fs10,66*4(sp)  
    flw fs11,67*4(sp)  
    flw ft8,68*4(sp)  
    flw ft9,69*4(sp)  
    flw ft10,70*4(sp)  
    flw ft11,71*4(sp) 
#endif

    addi sp,  sp, SWITCH_STACK_SIZE
    mret

/*
 * void rt_hw_context_switch_interrupt(rt_uint32 from, rt_uint32 to);
 */
    .globl rt_thread_switch_interrupt_flag
    .globl rt_interrupt_from_thread
    .globl rt_interrupt_to_thread
    .globl rt_hw_context_switch_interrupt
rt_hw_context_switch_interrupt:
    addi sp, sp, -16
    sw   s0, 12(sp)
    sw   a0, 8(sp)
    sw   a5, 4(sp)

    la   a0, rt_thread_switch_interrupt_flag
    lw   a5, (a0)
    bnez a5, _reswitch
    li   a5, 1
    sw   a5, (a0)

    la   a5, rt_interrupt_from_thread
    lw   a0, 8(sp)
    sw   a0, (a5)

_reswitch:
    la   a5, rt_interrupt_to_thread
    sw   a1, (a5)

    lw   a5, 4(sp)
    lw   a0, 8(sp)
    lw   s0, 12(sp)
    addi sp, sp, 16
    ret
