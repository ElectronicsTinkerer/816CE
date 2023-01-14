# 816CE: A 65816 Emulator/Debugging Assistant

## USAGE

## TIPS

* When using a memory watch in disassembly mode, the disassembly of immediate operand widths is based on the current state of the CPU's register widths. This means that the assembly output may be incorrect because a 2-byte instruction might be seen as a 3-byte instruction (such as `lda #$10`) if M is 0. A way to avoid this behavior is by running the disassembly mode in 'pc follow mode' by running the command `mw[1|2] asm pc`.

