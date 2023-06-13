#!/bin/python3

import json
import sys

if __name__ == "__main__":
    with open("output.scpu", "w") as scpufp:
        for i in sys.argv[1:]:
            with open(i, "r") as jsonfp:

                data = json.loads(jsonfp.read())

                for test_data in data:
                    test_name = test_data['name'] + "\n"
                    ci = test_data['initial']
                    cf = test_data['final']
                    
                    cpu_istr = f"{{C:{ci['a']:x},X:{ci['x']:x},Y:{ci['y']:x},SP:{ci['s']:x},D:{ci['d']:x},DBR:{ci['dbr']:x},PBR:{ci['pbr']:x},PC:{ci['pc']:x},RST:0,IRQ:0,NMI:0,STP:0,CRASH:0,PSC:{(ci['p']>>0)&0x1:x},PSZ:{(ci['p']>>1)&0x1:x},PSI:{(ci['p']>>2)&0x1:x},PSD:{(ci['p']>>3)&0x1:x},PSXB:{(ci['p']>>4)&0x1:x},PSM:{(ci['p']>>5)&0x1:x},PSV:{(ci['p']>>6)&0x1:x},PSN:{(ci['p']>>7)&0x1:x},PSE:{ci['e']:x},cycles:0}}\n"
                    cpu_fstr = f"{{C:{cf['a']:x},X:{cf['x']:x},Y:{cf['y']:x},SP:{cf['s']:x},D:{cf['d']:x},DBR:{cf['dbr']:x},PBR:{cf['pbr']:x},PC:{cf['pc']:x},RST:0,IRQ:0,NMI:0,STP:0,CRASH:0,PSC:{(cf['p']>>0)&0x1:x},PSZ:{(cf['p']>>1)&0x1:x},PSI:{(cf['p']>>2)&0x1:x},PSD:{(cf['p']>>3)&0x1:x},PSXB:{(cf['p']>>4)&0x1:x},PSM:{(cf['p']>>5)&0x1:x},PSV:{(cf['p']>>6)&0x1:x},PSN:{(cf['p']>>7)&0x1:x},PSE:{cf['e']:x},cycles:{len(test_data['cycles'])}}}\n"
                    
                    scpufp.write(f"TEST: {test_name}")
                    for r in ci['ram']:
                        scpufp.write(f"i:{r[0]:x}:{r[1]:x}\n")
                    for r in cf['ram']:
                        scpufp.write(f"f:{r[0]:x}:{r[1]:x}\n")
                    scpufp.write(cpu_istr)
                    scpufp.write(cpu_fstr)
    
