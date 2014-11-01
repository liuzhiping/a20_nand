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

#define PORTC_BASE			0x01c20800
#define CCMU_BASE			0x01c20000
#define NANDFLASHC_BASE			0x01c03000
#define DMAC_BASE			0x01c02000

#define NANDFLASHC_CTL			0x00000000
#define NANDFLASHC_ST			0x00000004
#define NANDFLASHC_INT			0x00000008
#define NANDFLASHC_TIMING_CTL		0x0000000C
#define NANDFLASHC_TIMING_CFG		0x00000010
#define NANDFLASHC_ADDR_LOW		0x00000014
#define NANDFLASHC_ADDR_HIGH		0x00000018
#define NANDFLASHC_SECTOR_NUM		0x0000001C
#define NANDFLASHC_CNT			0x00000020
#define NANDFLASHC_CMD			0x00000024
#define NANDFLASHC_RCMD_SET		0x00000028
#define NANDFLASHC_WCMD_SET		0x0000002C
#define NANDFLASHC_IO_DATA		0x00000030
#define NANDFLASHC_ECC_CTL		0x00000034
#define NANDFLASHC_ECC_ST		0x00000038
#define NANDFLASHC_DEBUG		0x0000003C
#define NANDFLASHC_ECC_CNT0		0x00000040
#define NANDFLASHC_ECC_CNT1		0x00000044
#define NANDFLASHC_ECC_CNT2		0x00000048
#define NANDFLASHC_ECC_CNT3		0x0000004C
#define NANDFLASHC_USER_DATA_BASE	0x00000050
#define NANDFLASHC_EFNAND_STATUS	0x00000090
#define NANDFLASHC_SPARE_AREA		0x000000A0
#define NANDFLASHC_PATTERN_ID		0x000000A4
#define NANDFLASHC_RAM0_BASE		0x00000400
#define NANDFLASHC_RAM1_BASE 		0x00000800


void nand_set_clocks(void) {
        W32(PORTC_BASE + 0x48, 0x22222222);
        W32(PORTC_BASE + 0x4C, 0x22222222);
        W32(PORTC_BASE + 0x50, 0x2222222);
        W32(PORTC_BASE + 0x54, 0x2);
        W32(PORTC_BASE + 0x5C, 0x55555555);
        W32(PORTC_BASE + 0x60, 0x15555);
        W32(PORTC_BASE + 0x64, 0x5140);
        W32(PORTC_BASE + 0x68, 0x4016);

        uint32_t val = R32(CCMU_BASE + 0x60);
        W32(CCMU_BASE + 0x60, 0x2000 | val);

        val = R32(CCMU_BASE + 0x80);
        W32(CCMU_BASE + 0x80, val | 0x80000000 | 0x1);
}

void nand_init(void) {
	uint32_t val;
	nand_set_clocks();
	// enable and reset
	val = R32(NANDFLASHC_BASE + 0x00);
	W32(NANDFLASHC_BASE + 0x00, val | 0x3);               // CTL       (1<<0 <-EN    1<<1 RESET)
        do {
                val = R32(NANDFLASHC_BASE + 0x00);
		if (val & (1<<1)) break;
        } while (1);
	printf("NAND: initialized\n");
}

// random seed used by linux
const uint16_t random_seed[128] = {
	0x2b75, 0x0bd0, 0x5ca3, 0x62d1, 0x1c93, 0x07e9, 0x2162, 0x3a72,
	0x0d67, 0x67f9, 0x1be7, 0x077d, 0x032f, 0x0dac, 0x2716, 0x2436,
	0x7922, 0x1510, 0x3860, 0x5287, 0x480f, 0x4252, 0x1789, 0x5a2d,
	0x2a49, 0x5e10, 0x437f, 0x4b4e, 0x2f45, 0x216e, 0x5cb7, 0x7130,
	0x2a3f, 0x60e4, 0x4dc9, 0x0ef0, 0x0f52, 0x1bb9, 0x6211, 0x7a56,
	0x226d, 0x4ea7, 0x6f36, 0x3692, 0x38bf, 0x0c62, 0x05eb, 0x4c55,
	0x60f4, 0x728c, 0x3b6f, 0x2037, 0x7f69, 0x0936, 0x651a, 0x4ceb,
	0x6218, 0x79f3, 0x383f, 0x18d9, 0x4f05, 0x5c82, 0x2912, 0x6f17,
	0x6856, 0x5938, 0x1007, 0x61ab, 0x3e7f, 0x57c2, 0x542f, 0x4f62,
	0x7454, 0x2eac, 0x7739, 0x42d4, 0x2f90, 0x435a, 0x2e52, 0x2064,
	0x637c, 0x66ad, 0x2c90, 0x0bad, 0x759c, 0x0029, 0x0986, 0x7126,
	0x1ca7, 0x1605, 0x386a, 0x27f5, 0x1380, 0x6d75, 0x24c3, 0x0f8e,
	0x2b7a, 0x1418, 0x1fd1, 0x7dc1, 0x2d8e, 0x43af, 0x2267, 0x7da3,
	0x4e3d, 0x1338, 0x50db, 0x454d, 0x764d, 0x40a3, 0x42e6, 0x262b,
	0x2d2e, 0x1aea, 0x2e17, 0x173d, 0x3a6e, 0x71bf, 0x25f9, 0x0a5d,
	0x7c57, 0x0fbe, 0x46ce, 0x4939, 0x6b17, 0x37bb, 0x3e91, 0x76db,
};

// read 0x400 bytes from real_addr to temp_buf
void nand_read_block(unsigned int real_addr, bool syndrome) {
	uint32_t val;
	memset((void*)&temp_buf, 0, 0x400); // clear temp_buf
	W32(NANDFLASHC_BASE + NANDFLASHC_CMD, 0xC000FF);
	do {
		val = R32(NANDFLASHC_BASE + NANDFLASHC_ST);
		if (!(val & 0x10)) break;
		udelay(1000);
	} while (1);

	uint32_t page = real_addr / (8 * 1024);
	uint32_t shift = real_addr % (8*1024);
	uint32_t rseed;
	rseed = syndrome ? 0x4A80 : random_seed[page % 128];
	W32(0x1C03034, (rseed << 16) | 0x0200); // ECC_CTL, randomization
	if (syndrome) {
		// shift every 1kB in syndrome
		shift += (shift / 0x400) * 0x2e;
	}
	uint32_t addr = (page << 16) | shift;

	val = R32(NANDFLASHC_BASE + NANDFLASHC_CTL);
	W32(NANDFLASHC_BASE + NANDFLASHC_CTL, val | 0x4001); // TODO
	W32(NANDFLASHC_BASE + NANDFLASHC_SPARE_AREA, 0x400);

	// DMAC
	W32(DMAC_BASE + 0x300, 0x0); // clr dma cmd
	W32(DMAC_BASE + 0x304, NANDFLASHC_BASE + NANDFLASHC_IO_DATA); // read from REG_IO_DATA
	W32(DMAC_BASE + 0x308, (uint32_t)&temp_buf); // read to RAM
	W32(DMAC_BASE + 0x318, 0x7F0F);
	W32(DMAC_BASE + 0x30C, 0x400); // 1kB
	W32(DMAC_BASE + 0x300, 0x84000423);

	W32(NANDFLASHC_BASE + NANDFLASHC_RCMD_SET, 0x00E00530);
	W32(NANDFLASHC_BASE + NANDFLASHC_SECTOR_NUM, 1);
	W32(NANDFLASHC_BASE + NANDFLASHC_ADDR_LOW, addr);
	W32(NANDFLASHC_BASE + NANDFLASHC_ADDR_HIGH, 0);

	W32(NANDFLASHC_BASE + NANDFLASHC_CMD, 0x87EC0000); // CMD (PAGE READ)

	do {
		val = R32(NANDFLASHC_BASE + NANDFLASHC_ST);
		if (val & (1<<2)) break; // wait for dma irq
		udelay(1000);
	} while (1);

	do {
		val = R32(DMAC_BASE + 300);
		if (!(val & 0x80000000)) break; // make sure cmd is finished
		udelay(1000);
	} while (1);
}

void nand_read(uint32_t addr, void *dest, int count) {
	uint32_t dst;
	uint32_t adr = addr;
	memset((void*)dest, 0x0, count); // clean destination memory
	for (dst = (uint32_t)dest; dst < ((uint32_t)dest+count); dst+=0x400) {
		nand_read_block(adr, adr < 0x400000); // if < 0x400000 then syndrome read
		memcpy((void*)dst, (void*)&temp_buf, 0x400);
		adr += 0x400;
	}
}

