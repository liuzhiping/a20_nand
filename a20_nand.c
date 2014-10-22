/*
Copyright (c) 2014, Antmicro Ltd <www.antmicro.com>
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// minimal "boot0" style NAND support for Allwinner A20

#define W32(a, b) (*(volatile unsigned int   *)(a)) = b
#define R32(a) (*(volatile unsigned int   *)(a))

// temporary buffer for read of 1024 bytes
char temp_buf[0x400] __attribute__ ((aligned (16)));

// #define printf(...)

void nand_set_clocks(void) {
        W32(0x1C20848, 0x22222222);
        W32(0x1C2084C, 0x22222222);
        W32(0x1C20850, 0x2222222);
        W32(0x1C20854, 0x2);
        W32(0x1C2085C, 0x55555555);
        W32(0x1C20860, 0x15555);
        W32(0x1C20864, 0x5140);
        W32(0x1C20868, 0x4016);

        uint32_t val = R32(0x1C20060);
        W32(0x1C20060, 0x2000 | val);

        val = R32(0x1C20080);
        W32(0x1C20080, val | 0x80000000 | 0x1);
}

void nand_init(void) {
	uint32_t val;
	printf("NAND: going to set clocks\n");
	nand_set_clocks();
	printf("NAND: going to perform reset\n");
	// enable and reset
	W32(0x1C03000, 0x3);               // CTL       (1<<0 <-EN    1<<1 RESET)
        do {
                val = R32(0x1C03000);
        } while (!(val & (1<<1)));
	printf("NAND: initialized\n");
}

void nand_read_block(unsigned int sector, unsigned int addr) {
	uint32_t val;
	printf("NAND: going to read sector %d @ addr = 0x%08X\n", sector, addr);
        memset(0, 0, 0x400); // clear RAM @ 0
	W32(0x1C03024, 0xC000FF);          // CM
        do {
                val = R32(0x1C03004);      // ST
		udelay(1000);
	} while (val & 0x10); // TODO
	
	val = R32(0x1C03034);              // ECC_CTL
	W32(0x1C03034, val |  0x8201);

	W32(0x1C03000, 0x4001);          // CTL, 0x4000 = RAM_METHOD
	W32(0x1C030A0, 0x400);           // SPARE_AREA
	W32(0x1C03000, 0x4001);
	W32(0x1C03028, 0xE00530);        // RCMD_SET

	// DMAC
	W32(0x1C02000, 0x0);
	W32(0x1C02304, 0x1C03030); // point to REG_IO_DATA
	W32(0x1C02308, (uint32_t)temp_buf); // point to RAM
	W32(0x1C02318, 0x7F0F);
	W32(0x1C0230C, 0x400);    // 1024 bytes
	W32(0x1C02300, 0x84000423);

	// TODO: set  SECTOR_NUM and ADDR_LOW/HIGH
	W32(0x1C0301C, sector);          // SECTOR_NUM
	W32(0x1C03014, addr);             // ADDR_LOW
	W32(0x1C03018, 0x0);             // ADDR_HIGH
	W32(0x1C03024, 0x87EC0000);      // CMD

	printf("NAND: waiting for DMA to finish\n");
        do {
                val = R32(0x1C03004);
                udelay(1000);
        } while (!(val & (1<<2))); // wait for dma irq

        do {
                val = R32(0x1C02300);
                udelay(1000);
        } while (val & 0x80000000); // make sure dma is done

	W32(0x1C02300, 0x0); // clean dma command
	udelay(1000);	
	W32(0x1C03034, 0x4A800008);      // ECC_CTL
}

void nand_read(uint32_t addr, uint32_t dest, int count) {
	uint32_t dst;
	uint32_t adr = addr;
	for (dst = dest; dst < (dest+count); dst+=0x400) {
		uint32_t sector = 1 + (adr / (2 * 1024 * 1024));
		uint32_t saddr = adr - ((sector-1) * 2 * 1024 * 1024);
		nand_read_block(sector, saddr);
		memcpy((void*)dst, (void*)temp_buf, 0x400);
		adr += 0x400;
	}
}

