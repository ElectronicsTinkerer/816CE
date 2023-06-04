# 816CE: A 65816 Emulator/Debugging Assistant and 65816 CPU Core

This project aims to provide two usable pieces of software/code to aid in the software development for systems based on the 65816 CPU. The first component is the 65816 core, which provides a simple API for stepping the 65816 core through its operation. The other component is a user interface built around this API which runs in the terminal.

![Screenshot of debugger in operation](./debugger.png)

## Files

Everything is written in C and should be compatible with any C compiler which supports C99 or newer. The 65816 core makes use of very few libraries (all from the C standard library headers) and should be portable to any platform that supports `uint32_t` sized variables. On the other hand, the simulator interface requires ncurses and sockets to operate.

* Any file in the `src` directory which starts with `65816` is part of the CPU core.
* The remaining files in the `src` directory are used for the simulation interface. `debugger.c` contains the `main()` function for the simulator.

## COMPILING

To build on a standard GNU/Linux system, make sure that `libncurses` is installed. Then run `make` in the repo's root directory. This should produce a binary in the `build` directory which can be run.

## USAGE

The simulator program can be invoked with or without arguments. The help menu is below:

```
65816 Simulator (C) Ray Clemens 2022-2023
USAGE:
 $ 816ce (--cpu filename) (--mem (offset) filename)

Args:
 --cpu-file filename ....... Preload the CPU with a saved state
 --mem (offset) filename ... Load memory at offset (in hex) with a file
 --mem-mos filename ........ Load a binary file formatted for the LLVM MOS simulator into memory
 --cmd "[command here]" .... Run a command during initialization
 --cmd-file filename ....... Run commands from a file during initialization
```

The `mem` and `cpu` arguments can be overridden during program execution by running the `load` command to load memory or CPU save states. Note that multiple memory files can be passed to be loaded in different memory regions based on the offset provided, which defaults to address 0. Multiple CPU save files can also be loaded, however, only the last file provided will be loaded.

Commands in a command file (specified by `cmd-file`) are newline separated, i.e., one command per line. There is a (large) maximum line length which will truncate commands if they are too long.

While the simulator is open, press `?` to access the command help menu.

```
Available commands
 > exit|quit
 > mw[1|2] [mem|asm] (pc|addr)
 > mw[1|2] aaaaaa
 > irq [set|clear]
 > nmi [set|clear]
 > aaaaaa: xx yy zz
 > save [mem|cpu] filename
 > load mem (mos) (offset) filename
 > load cpu filename
 > cpu [reg] xxxx
 > cpu [option] [enable|disable|status]
 > br aaaaaa
 > uart [type] aaaaaa (pppp)
 ? ... Help Menu
 ^G to clear command input
```

Some explanation:

* `[cat|dog]` - either `cat` or `dog` can be entered but the field is required
* `(value)` - an optional field
* `aaaaaa` - an address in hex
* `reg` - a CPU register in all caps (e.g. PC)
* `type` - for a `uart` initialization, the type refers to the HW being emulated (see below)
* `pppp` - A port number in decimal (if 0, then the uart is disabled)
* `option` - A CPU option to change CPU behavior (see below)

Additionally, the function keys are of use:

```
F1  - Toggle breakpoint at current CPU PC
F2  - Toggle IRQ on CPU
F3  - Toggle NMI on CPU
F4  - Halt CPU
F5  - Run until Halt pressed, CPU CRASH, CPU executes STP, or a breakpoint is hit
F6  - Skip instruction at current PC
F7  - Step by one instruction
F9  - Reset CPU
F12 - Pressing F12 twice will exit the simulator without saving.
^X^C  - Close simulator without saving.
ESC-Q - Close simulator without closing
```

### File loading & saving

* Files can be specified to be loaded into memory and/or the CPU via arguments to the simulator or during runtime by using the `load` command.
* Loaded files are not automatically saved upon termination of the simulator.
* Memory contents and CPU state can be manually saved through the use of the `save` command

### UART Types

When issuing the `uart` command, the `type` argument can refer to the following uart devices:
* `c750` - TL16C750

UART devices listen on a TCP socket to implement a serial port-like behavior. For example, if the command `uart c750 4840 6500` is executed and succeeds, the UART will be listening on port 6500 for TCP connections. The UART will also update the DCD (Data Carrier Detect) flag to show connection status. (Currently, writing to the UART is required to detect if the port has been disconnected.) The TCP port can be connected to via netcat (e.g. `stty -icanon && nc localhost 6500` - with the `stty` being necessary to make sure text is not line buffered.)

Additionally, only one instance of a UART is currently supported. If the `uart` command is executed after a previous `uart` command, the previous TCP sockets are closed and a new socket listener is created. The UART is also connected to the CPU's IRQ line so if interrupts are enabled on the UART and an interrupt condition occurs, the CPU will be signaled. Since the UART now controlls the IRQ line, the F2 IRQ toggle has no effect.

### CPU Options

CPU options are features of the CPU that are not necessarily implemented by a stock CPU but may be handy for use in the simulator. Here are the currently available options:
* `cop` - If enabled, the immediate byte to the COP instruction will be used as an offset into a table who's base is the value of the COP vector (depends on emulation mode). For example, a COP vector of `$8000` and the instruction `COP $02` would cause the CPU to jump to the value stored at `$8000 + ($02 << 1)` = `$8004`. If memory location `$8004..$8005` contained the value `$c0e0`, then the CPU would jump to `$c0e0`. Otherwise, all COP-related functionality remains the same.

## TIPS

* When using a memory watch in disassembly mode, the disassembly of immediate operand widths is based on the current state of the CPU's register widths. This means that the assembly output may be incorrect because a 2-byte instruction might be seen as a 3-byte instruction (such as `lda #$10`) if M is 0. A way to avoid this behavior is by running the disassembly mode in 'pc follow mode' by running the command `mw[1|2] asm pc`.

