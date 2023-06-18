/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */

#ifndef OPS_65816_H
#define OPS_65816_H

#include "65816.h"
#include "65816-util.h"

void i_adc(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_and(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_asl(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_bcc(CPU_t *, memory_t *);
void i_bcs(CPU_t *, memory_t *);
void i_beq(CPU_t *, memory_t *);
void i_bit(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_bmi(CPU_t *, memory_t *);
void i_bne(CPU_t *, memory_t *);
void i_bpl(CPU_t *, memory_t *);
void i_bra(CPU_t *, memory_t *);
void i_brk(CPU_t *, memory_t *);
void i_brl(CPU_t *, memory_t *);
void i_bvc(CPU_t *, memory_t *);
void i_bvs(CPU_t *, memory_t *);
void i_clc(CPU_t *);
void i_cld(CPU_t *);
void i_cli(CPU_t *);
void i_clv(CPU_t *);
void i_cmp(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_cop(CPU_t *, memory_t *);
void i_cpx(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_cpy(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_dea(CPU_t *);
void i_dec(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_dex(CPU_t *);
void i_dey(CPU_t *);
void i_eor(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_ina(CPU_t *);
void i_inc(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_inx(CPU_t *);
void i_iny(CPU_t *);
void i_jmp(CPU_t *, memory_t *, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_jsr(CPU_t *, memory_t *, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_jsl(CPU_t *, memory_t *, uint8_t, uint32_t);
void i_lda(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_ldx(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_ldy(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_lsr(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_mvn(CPU_t *, memory_t *);
void i_mvp(CPU_t *, memory_t *);
void i_nop(CPU_t *);
void i_ora(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_pea(CPU_t *, memory_t *);
void i_pei(CPU_t *, memory_t *);
void i_per(CPU_t *, memory_t *);
void i_pha(CPU_t *, memory_t *);
void i_phb(CPU_t *, memory_t *);
void i_phk(CPU_t *, memory_t *);
void i_php(CPU_t *, memory_t *);
void i_phx(CPU_t *, memory_t *);
void i_phy(CPU_t *, memory_t *);
void i_phd(CPU_t *, memory_t *);
void i_pla(CPU_t *, memory_t *);
void i_plb(CPU_t *, memory_t *);
void i_pld(CPU_t *, memory_t *);
void i_plp(CPU_t *, memory_t *);
void i_plx(CPU_t *, memory_t *);
void i_ply(CPU_t *, memory_t *);
void i_rep(CPU_t *, memory_t *);
void i_rol(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_ror(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_rti(CPU_t *, memory_t *);
void i_rtl(CPU_t *, memory_t *);
void i_rts(CPU_t *, memory_t *);
void i_sbc(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_sec(CPU_t *);
void i_sed(CPU_t *);
void i_sei(CPU_t *);
void i_sep(CPU_t *, memory_t *);
void i_stp(CPU_t *);
void i_sta(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_stx(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_sty(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_stz(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_tax(CPU_t *);
void i_tay(CPU_t *);
void i_tcd(CPU_t *);
void i_tcs(CPU_t *);
void i_tdc(CPU_t *);
void i_trb(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_tsb(CPU_t *, memory_t *, uint8_t, uint8_t, CPU_Addr_Mode_t, uint32_t);
void i_tsc(CPU_t *);
void i_tsx(CPU_t *);
void i_txa(CPU_t *);
void i_txs(CPU_t *);
void i_txy(CPU_t *);
void i_tya(CPU_t *);
void i_tyx(CPU_t *);
void i_wai(CPU_t *);
void i_wdm(CPU_t *);
void i_xba(CPU_t *);
void i_xce(CPU_t *);

#endif
