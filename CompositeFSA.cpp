#include "CompositeVideo.h"
#include "stdio.h"
//	(C) 2022 Matthew Regan
//	Provided as is, no guarantees provided.
//	Free to use under GNU GPLv3  - https://www.gnu.org/licenses/gpl-3.0.html
//
//	YouTube video explaining this code can be found at https://youtu.be/32Y-Pw8jyjY

#define VPORCHBASEADDRESS			0x00000
#define ACTIVEBASEADDRESS			0x02000
#define HPORCHBASEADDRESS			0x06000
#define VSYNCBASEADDRESS			0x0FA00
#define TOTALWIDTH					65
#define ACTIVEWIDTH					40
#define VPORCHHEIGHT				70
#define TOTALHEIGHT					262
#define HORIZONTALSYNCPOSITION		50
#define HORIZONTALSYNCWIDTH			5
#define VERTICALSYNCPOSITION		31
#define VERTICALSYNCWIDTH			8

//	Composite Video layout for high-res mode 1 for the Apple ][+
//	Finite state machine regions.  
//	
//		<---------------------- 65 ----------------------------->
//
//		---------------------------------------------------------
//      | 0x0000     70x40 = 2800               | 0x6000        |
// 70   |           Vertical Porch              |  25*262       |
//      |                                       |   = 6550      |
//		|---------------------------------------|  Horizontal   |
//      | 0x2000                                |    Porch      |
//      |           40*192 = 7680               |               |
// 192  |            Active Area                |               |
//      |                                       |               |
//      |                                       |               |
//      |                                       |               |
//		---------------------------------------------------------


unsigned int CompositeFSA[ROMSIZE];

unsigned int SwizzleData(unsigned int val) 
{
	unsigned int ret = 0;
	for (int i = 0; i < 16; i++) {
		if (val&(1<<i)) ret |= ((i & 1) ? (1 << (((i & 14) >> 1) + 8)) : (1 << ((i >> 1))));
	}
	for (int i = 16; i < 32; i++) {
		if (val & (1 << i)) ret |= ((i & 1) ? (1 << (((i & 14) >> 1) + 24)) : (1 << ((i >> 1)+16)));
	}
	return ret;
}

//	Compute the starting address of a given line within a high-res page.
unsigned int GenerateLineAddress(int line)
{
	unsigned int address;
	address = 0;
	address += (line & 0x07) * 0x400;			//	Magic numbers derived from Inside Apple IIe
	address += ((line >> 3) & 0x07) * 0x080;
	address += ((line >> 6) & 0x03) * 0x028;
	return address;
}

//	Make the finite state generator
void	GenerateCompositeFSA()
{
	//	Increment current location.
	for (unsigned int i = 0; i < ROMSIZE; i++) {
		CompositeFSA[i] = (i + 1) & 0xffff;
	}

	//	Perform Left Stitch
	//	Stitch vertical porch to horizontal porch
	for (unsigned int i = 0; i < VPORCHHEIGHT; i++) {
		CompositeFSA[VPORCHBASEADDRESS + i * ACTIVEWIDTH + ACTIVEWIDTH - 1] =
			HPORCHBASEADDRESS + i * (TOTALWIDTH - ACTIVEWIDTH);
	}
	//	Stitch active display area to horizontal porch
	for (unsigned int i = VPORCHHEIGHT; i< TOTALHEIGHT; i++) {
		CompositeFSA[ACTIVEBASEADDRESS + GenerateLineAddress(i- VPORCHHEIGHT) + ACTIVEWIDTH - 1] =
			HPORCHBASEADDRESS + i * (TOTALWIDTH - ACTIVEWIDTH);
	}

	//	Perform Right Stitch
	//	Stitch horizontal porch to vertical porch
	for (unsigned int i = 0; i < VPORCHHEIGHT; i++) {
		CompositeFSA[HPORCHBASEADDRESS + (i+1) * (TOTALWIDTH - ACTIVEWIDTH) - 1] =
			VPORCHBASEADDRESS + (i+1) * (ACTIVEWIDTH);
	}
	//	Stitch horizontal porch to active area
	for (unsigned int i = VPORCHHEIGHT; i < TOTALHEIGHT; i++) {
		CompositeFSA[HPORCHBASEADDRESS + (i + 1) * (TOTALWIDTH - ACTIVEWIDTH) - 1] =
			ACTIVEBASEADDRESS + GenerateLineAddress(i+1 - VPORCHHEIGHT);
	}

	//	Now perform two final stitches.
	//	Wrap onto active area instead of vertical porch
	CompositeFSA[HPORCHBASEADDRESS + (VPORCHHEIGHT) * (TOTALWIDTH - ACTIVEWIDTH) - 1] =
		ACTIVEBASEADDRESS + GenerateLineAddress(0);

	//	Wrap onto vertical porch instead of active area
	CompositeFSA[HPORCHBASEADDRESS + (TOTALHEIGHT) * (TOTALWIDTH - ACTIVEWIDTH) - 1] =
		VPORCHBASEADDRESS;

	//	Copy lower FSA to upper locations
	for (unsigned int i = 0; i < ROMSIZE / 2; i++) {
		CompositeFSA[i + ROMSIZE / 2] = CompositeFSA[i] + ROMSIZE / 2;
	}

	//	Add in Horizontal Sync
	for (unsigned int i = 0; i < TOTALHEIGHT; i++) {
		CompositeFSA[HPORCHBASEADDRESS + i * (TOTALWIDTH - ACTIVEWIDTH) + HORIZONTALSYNCPOSITION - ACTIVEWIDTH]
			= ROMSIZE / 2 + HPORCHBASEADDRESS + i * (TOTALWIDTH - ACTIVEWIDTH) + HORIZONTALSYNCPOSITION - ACTIVEWIDTH + 1;
	}
	for (unsigned int i = 0; i < TOTALHEIGHT; i++) {
		CompositeFSA[ROMSIZE / 2 + HPORCHBASEADDRESS + i * (TOTALWIDTH - ACTIVEWIDTH) + HORIZONTALSYNCPOSITION - ACTIVEWIDTH + HORIZONTALSYNCWIDTH]
			= HPORCHBASEADDRESS + i * (TOTALWIDTH - ACTIVEWIDTH) + HORIZONTALSYNCPOSITION - ACTIVEWIDTH + HORIZONTALSYNCWIDTH + 1;
	}

	//	Link horizontal window to VSYNC region
	CompositeFSA[HPORCHBASEADDRESS + VERTICALSYNCPOSITION * (TOTALWIDTH - ACTIVEWIDTH) - 1] = VSYNCBASEADDRESS;

	//	Like VSYNC back to Vertical Porch
	CompositeFSA [VSYNCBASEADDRESS + VERTICALSYNCWIDTH * TOTALWIDTH - 1]
		= VPORCHBASEADDRESS + (VERTICALSYNCPOSITION + VERTICALSYNCWIDTH) * (ACTIVEWIDTH);

	//	Write this data into the EPROM
	FILE* fp;
	fopen_s(&fp, "Composite322.ROM", "w+b"); 
	if (!fp) return;

	for (unsigned int i = 0; i < ROMSIZE; i++) {
		unsigned int value;
		value = SwizzleData(CompositeFSA[i]);
		fputc(value & 0xff, fp);
		fputc((value >> 8) & 0xff, fp);
	}
	fclose(fp);


}