// For license and other informations see LedDeviceWS2812b.h
// To activate: use led device "ws2812s" in the hyperion configuration

// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>

// Linux includes
#include <fcntl.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/ioctl.h>

#ifdef BENCHMARK
	#include <time.h>
#endif

// hyperion local includes
#include "LedDeviceWS2812b.h"

// ==== Defines and Vars ====

// Base addresses for GPIO, PWM, PWM clock, and DMA controllers (physical, not bus!)
// These will be "memory mapped" into virtual RAM so that they can be written and read directly.
// -------------------------------------------------------------------------------------------------
#define DMA_BASE		0x20007000
#define DMA_LEN			0x24
#define PWM_BASE		0x2020C000
#define PWM_LEN			0x28
#define CLK_BASE	    0x20101000
#define CLK_LEN			0xA8
#define GPIO_BASE		0x20200000
#define GPIO_LEN		0xB4

// GPIO
// -------------------------------------------------------------------------------------------------
#define GPFSEL0			0x20200000			// GPIO function select, pins 0-9 (bits 30-31 reserved)
#define GPFSEL1			0x20200004			// Pins 10-19
#define GPFSEL2			0x20200008			// Pins 20-29
#define GPFSEL3			0x2020000C			// Pins 30-39
#define GPFSEL4			0x20200010			// Pins 40-49
#define GPFSEL5			0x20200014			// Pins 50-53
#define GPSET0			0x2020001C			// Set (turn on) pin
#define GPCLR0			0x20200028			// Clear (turn off) pin
#define GPPUD			0x20200094			// Internal pullup/pulldown resistor control
#define GPPUDCLK0		0x20200098			// PUD clock for pins 0-31
#define GPPUDCLK1		0x2020009C			// PUD clock for pins 32-53

// Memory offsets for the PWM clock register, which is undocumented! Please fix that, Broadcom!
// -------------------------------------------------------------------------------------------------
#define	PWM_CLK_CNTL 	40		// Control (on/off)
#define	PWM_CLK_DIV  	41		// Divisor (bits 11:0 are *quantized* floating part, 31:12 integer part)

// PWM Register Addresses (page 141)
// These are divided by 4 because the register offsets in the guide are in bytes (8 bits) but
// the pointers we use in this program are in words (32 bits). Buss' original defines are in
// word offsets, e.g. PWM_RNG1 was 4 and PWM_DAT1 was 5. This is functionally the same, but it
// matches the numbers supplied in the guide.
// -------------------------------------------------------------------------------------------------
#define	PWM_CTL  0x00		// Control Register
#define PWM_STA  (0x04 / 4)	// Status Register
#define PWM_DMAC (0x08 / 4)	// DMA Control Register
#define PWM_RNG1 (0x10 / 4)	// Channel 1 Range
#define PWM_DAT1 (0x14 / 4)	// Channel 1 Data
#define PWM_FIF1 (0x18 / 4)	// FIFO (for both channels - bytes are interleaved if both active)
#define PWM_RNG2 (0x20 / 4)	// Channel 2 Range
#define PWM_DAT2 (0x24 / 4)	// Channel 2 Data

// PWM_CTL register bit offsets
// Note: Don't use MSEN1/2 for this purpose. It will screw things up.
// -------------------------------------------------------------------------------------------------
#define PWM_CTL_MSEN2	15	// Channel 2 - 0: Use PWM algorithm. 1: Use M/S (serial) algorithm.
#define PWM_CTL_USEF2	13	// Channel 2 - 0: Use PWM_DAT2. 1: Use FIFO.
#define PWM_CTL_POLA2	12	// Channel 2 - Invert output polarity (if set, 0=high and 1=low)
#define PWM_CTL_SBIT2	11	// Channel 2 - Silence bit (default line state when not transmitting)
#define PWM_CTL_RPTL2	10	// Channel 2 - Repeat last data in FIFO
#define PWM_CTL_MODE2	9	// Channel 2 - Mode. 0=PWM, 1=Serializer
#define PWM_CTL_PWEN2	8	// Channel 2 - Enable PWM
#define	PWM_CTL_CLRF1	6	// Clear FIFO
#define	PWM_CTL_MSEN1	7	// Channel 1 - 0: Use PWM algorithm. 1: Use M/S (serial) algorithm.
#define	PWM_CTL_USEF1	5	// Channel 1 - 0: Use PWM_DAT1. 1: Use FIFO.
#define	PWM_CTL_POLA1	4	// Channel 1 - Invert output polarity (if set, 0=high and 1=low)
#define	PWM_CTL_SBIT1	3	// Channel 1 - Silence bit (default line state when not transmitting)
#define	PWM_CTL_RPTL1	2	// Channel 1 - Repeat last data in FIFO
#define	PWM_CTL_MODE1	1	// Channel 1 - Mode. 0=PWM, 1=Serializer
#define	PWM_CTL_PWEN1	0	// Channel 1 - Enable PWM

// PWM_STA register bit offsets
// -------------------------------------------------------------------------------------------------
#define PWM_STA_STA4	12	// Channel 4 State
#define PWM_STA_STA3	11	// Channel 3 State
#define PWM_STA_STA2	10	// Channel 2 State
#define PWM_STA_STA1	9	// Channel 1 State
#define PWM_STA_BERR	8	// Bus Error
#define PWM_STA_GAPO4	7	// Gap Occurred on Channel 4
#define PWM_STA_GAPO3	6	// Gap Occurred on Channel 3
#define PWM_STA_GAPO2	5	// Gap Occurred on Channel 2
#define PWM_STA_GAPO1	4	// Gap Occurred on Channel 1
#define PWM_STA_RERR1	3	// FIFO Read Error
#define PWM_STA_WERR1	2	// FIFO Write Error
#define PWM_STA_EMPT1	1	// FIFO Empty
#define PWM_STA_FULL1	0	// FIFO Full

// PWM_DMAC bit offsets
// -------------------------------------------------------------------------------------------------
#define PWM_DMAC_ENAB	31	// 0: DMA Disabled. 1: DMA Enabled.
#define PWM_DMAC_PANIC	8	// Bits 15:8. Threshold for PANIC signal. Default 7.
#define PWM_DMAC_DREQ	0	// Bits 7:0. Threshold for DREQ signal. Default 7.

// PWM_RNG1, PWM_RNG2
// --------------------------------------------------------------------------------------------------
// Defines the transmission range. In PWM mode, evenly spaced pulses are sent within a period
// of length defined in these registers. In serial mode, serialized data is sent within the
// same period. The value is normally 32. If less, data will be truncated. If more, data will
// be padded with zeros.

// DAT1, DAT2
// --------------------------------------------------------------------------------------------------
// NOTE: These registers are not useful for our purposes - we will use the FIFO instead!
// Stores 32 bits of data to be sent when USEF1/USEF2 is 0. In PWM mode, defines how many
// pulses will be sent within the period specified in PWM_RNG1/PWM_RNG2. In serializer mode,
// defines a 32-bit word to be transmitted.

// FIF1
// --------------------------------------------------------------------------------------------------
// 32-bit-wide register used to "stuff" the FIFO, which has 16 32-bit words. (So, if you write
// it 16 times, it will fill the FIFO.)
// See also:	PWM_STA_EMPT1 (FIFO empty)
//				PWM_STA_FULL1 (FIFO full)
//				PWM_CTL_CLRF1 (Clear FIFO)

// DMA
// --------------------------------------------------------------------------------------------------
// DMA registers (divided by four to convert form word to byte offsets, as with the PWM registers)
#define DMA_CS				(0x00 / 4)	// Control & Status register
#define DMA_CONBLK_AD		(0x04 /	4)	// Address of Control Block (must be 256-BYTE ALIGNED!!!)
#define DMA_TI				(0x08 /	4)	// Transfer Information (populated from CB)
#define DMA_SOURCE_AD		(0x0C /	4)	// Source address, populated from CB. Physical address.
#define DMA_DEST_AD			(0x10 /	4)	// Destination address, populated from CB. Bus address.
#define DMA_TXFR_LEN		(0x14 /	4)	// Transfer length, populated from CB
#define DMA_STRIDE			(0x18 /	4)	// Stride, populated from CB
#define DMA_NEXTCONBK		(0x1C /	4)	// Next control block address, populated from CB
#define DMA_DEBUG			(0x20 /	4)	// Debug settings

// DMA Control & Status register bit offsets
#define DMA_CS_RESET		31			// Reset the controller for this channel
#define DMA_CS_ABORT		30			// Set to abort transfer
#define DMA_CS_DISDEBUG		29			// Disable debug pause signal
#define DMA_CS_WAIT_FOR		28			// Wait for outstanding writes
#define DMA_CS_PANIC_PRI	20			// Panic priority (bits 23:20), default 7
#define DMA_CS_PRIORITY		16			// AXI priority level (bits 19:16), default 7
#define DMA_CS_ERROR		8			// Set when there's been an error
#define DMA_CS_WAITING_FOR	6			// Set when the channel's waiting for a write to be accepted
#define DMA_CS_DREQ_STOPS_DMA 5			// Set when the DMA is paused because DREQ is inactive
#define DMA_CS_PAUSED		4			// Set when the DMA is paused (active bit cleared, etc.)
#define DMA_CS_DREQ			3			// Set when DREQ line is high
#define DMA_CS_INT			2			// If INTEN is set, this will be set on CB transfer end
#define DMA_CS_END			1			// Set when the current control block is finished
#define DMA_CS_ACTIVE		0			// Enable DMA (CB_ADDR must not be 0)
// Default CS word
#define DMA_CS_CONFIGWORD	(8 << DMA_CS_PANIC_PRI) | \
							(8 << DMA_CS_PRIORITY) | \
							(1 << DMA_CS_WAIT_FOR)

// DREQ lines (page 61, most DREQs omitted)
#define DMA_DREQ_ALWAYS		0
#define DMA_DREQ_PCM_TX		2
#define DMA_DREQ_PCM_RX		3
#define DMA_DREQ_PWM		5
#define DMA_DREQ_SPI_TX		6
#define DMA_DREQ_SPI_RX		7
#define DMA_DREQ_BSC_TX		8
#define DMA_DREQ_BSC_RX		9

// DMA Transfer Information register bit offsets
// We don't write DMA_TI directly. It's populated from the TI field in a control block.
#define DMA_TI_NO_WIDE_BURSTS	26		// Don't do wide writes in 2-beat bursts
#define DMA_TI_WAITS			21		// Wait this many cycles after end of each read/write
#define DMA_TI_PERMAP			16		// Peripheral # whose ready signal controls xfer rate (pwm=5)
#define DMA_TI_BURST_LENGTH		12		// Length of burst in words (bits 15:12)
#define DMA_TI_SRC_IGNORE		11		// Don't perform source reads (for fast cache fill)
#define DMA_TI_SRC_DREQ			10		// Peripheral in PERMAP gates source reads
#define DMA_TI_SRC_WIDTH		9		// Source transfer width - 0=32 bits, 1=128 bits
#define DMA_TI_SRC_INC			8		// Source address += SRC_WITH after each read
#define DMA_TI_DEST_IGNORE		7		// Don't perform destination writes
#define DMA_TI_DEST_DREQ		6		// Peripheral in PERMAP gates destination writes
#define DMA_TI_DEST_WIDTH		5		// Destination transfer width - 0=32 bits, 1=128 bits
#define DMA_TI_DEST_INC			4		// Dest address += DEST_WIDTH after each read
#define DMA_TI_WAIT_RESP		3		// Wait for write response
#define DMA_TI_TDMODE			1		// 2D striding mode
#define DMA_TI_INTEN			0		// Interrupt enable
// Default TI word
#define DMA_TI_CONFIGWORD		(1 << DMA_TI_NO_WIDE_BURSTS) | \
								(1 << DMA_TI_SRC_INC) | \
								(1 << DMA_TI_DEST_DREQ) | \
								(1 << DMA_TI_WAIT_RESP) | \
								(1 << DMA_TI_INTEN) | \
								(DMA_DREQ_PWM << DMA_TI_PERMAP)

// DMA Debug register bit offsets
#define DMA_DEBUG_LITE					28		// Whether the controller is "Lite"
#define DMA_DEBUG_VERSION				25		// DMA Version (bits 27:25)
#define DMA_DEBUG_DMA_STATE				16		// DMA State (bits 24:16)
#define DMA_DEBUG_DMA_ID				8		// DMA controller's AXI bus ID (bits 15:8)
#define DMA_DEBUG_OUTSTANDING_WRITES	4		// Outstanding writes (bits 7:4)
#define DMA_DEBUG_READ_ERROR			2		// Slave read response error (clear by setting)
#define DMA_DEBUG_FIFO_ERROR			1		// Operational read FIFO error (clear by setting)
#define DMA_DEBUG_READ_LAST_NOT_SET		0		// AXI bus read last signal not set (clear by setting)



#define PAGE_SIZE	4096					// Size of a RAM page to be allocated
#define PAGE_SHIFT	12						// This is used for address translation
#define NUM_PAGES	((sizeof(struct control_data_s) + PAGE_SIZE - 1) >> PAGE_SHIFT)

#define SETBIT(word, bit) word |= 1<<bit
#define CLRBIT(word, bit) word &= ~(1<<bit)
#define GETBIT(word, bit) word & (1 << bit) ? 1 : 0
#define true 1
#define false 0

// GPIO
#define INP_GPIO(g) *(gpio_reg+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio_reg+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio_reg+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET *(gpio_reg+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio_reg+10) // clears bits which are 1 ignores bits which are 0



LedDeviceWS2812b::LedDeviceWS2812b() :
	LedDevice(),
	mLedCount(0)

#ifdef BENCHMARK
	,
	runCount(0),
	combinedNseconds(0),
	shortestNseconds(2147483647)
#endif

{
	//shortestNseconds = 2147483647;
	// Init PWM generator and clear LED buffer
	initHardware();
	//clearLEDBuffer();

	// init bit pattern, it is always 1X0
	unsigned int wireBit = 0;

	while ((wireBit + 3) < ((NUM_DATA_WORDS) * 4 * 8))
	{
		setPWMBit(wireBit++, 1);
		setPWMBit(wireBit++, 0); // just init it with 0
		setPWMBit(wireBit++, 0);
	}

	printf("WS2812b init finished \n");
}

#ifdef WS2812_ASM_OPTI

// rotate register, used to move the 1 around :-)
static inline __attribute__((always_inline)) uint32_t arm_ror_imm(uint32_t v, uint32_t sh)
{
	uint32_t d;
	asm ("ROR %[Rd], %[Rm], %[Is]" : [Rd] "=r" (d) : [Rm] "r" (v), [Is] "r" (sh));
	return d;
}

// rotate register, used to move the 1 around, add 1 to int counter on carry
static inline __attribute__((always_inline)) uint32_t arm_ror_imm_add_on_carry(uint32_t v, uint32_t sh, uint32_t inc)
{
	  uint32_t d;
	  asm ("RORS %[Rd], %[Rm], %[Is]\n\t"
		   "ADDCS %[Rd1], %[Rd1], #1"
			  : [Rd] "=r" (d), [Rd1] "+r" (inc): [Rm] "r" (v), [Is] "r" (sh));
	  return d;
}

static inline __attribute__((always_inline)) uint32_t arm_ror(uint32_t v, uint32_t sh)
{
	  uint32_t d;
	  asm ("ROR %[Rd], %[Rm], %[Rs]" : [Rd] "=r" (d) : [Rm] "r" (v), [Rs] "r" (sh));
	  return d;
}


static inline __attribute__((always_inline)) uint32_t arm_Bit_Clear_imm(uint32_t v, uint32_t v2)
{
	  uint32_t d;
	  asm ("BIC %[Rd], %[Rm], %[Rs]" : [Rd] "=r" (d) : [Rm] "r" (v), [Rs] "r" (v2));
	  return d;
}
#endif

int LedDeviceWS2812b::write(const std::vector<ColorRgb> &ledValues)
{
#ifdef BENCHMARK
	timespec timeStart;
	timespec timeEnd;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeStart);
#endif

	mLedCount = ledValues.size();

	// Read data from LEDBuffer[], translate it into wire format, and write to PWMWaveform
	unsigned int colorBits = 0;			// Holds the GRB color before conversion to wire bit pattern
	unsigned int wireBit = 1;			// Holds the current bit we will set in PWMWaveform, start with 1 and skip the other two for speed

	// Copy PWM waveform to DMA's data buffer
	//printf("Copying %d words to DMA data buffer\n", NUM_DATA_WORDS);
	struct control_data_s *ctl = (struct control_data_s *)virtbase;
	dma_cb_t *cbp = ctl->cb;

	// 72 bits per pixel / 32 bits per word = 2.25 words per pixel
	// Add 1 to make sure the PWM FIFO gets the message: "we're sending zeroes"
	// Times 4 because DMA works in bytes, not words
	cbp->length = ((mLedCount * 2.25) + 1) * 4;
	if(cbp->length > NUM_DATA_WORDS * 4)
	{
		cbp->length = NUM_DATA_WORDS * 4;
		mLedCount = (NUM_DATA_WORDS - 1) / 2.25;
	}

#ifdef WS2812_ASM_OPTI
	unsigned int startbitPattern = 0x40000000; // = 0100 0000  0000 0000  0000 0000  0000 0000 pattern
#endif


	for(size_t i=0; i<mLedCount; i++)
	{
		// Create bits necessary to represent one color triplet (in GRB, not RGB, order)
		//printf("RGB: %d, %d, %d\n", ledValues[i].red, ledValues[i].green, ledValues[i].blue);
		colorBits = ((unsigned int)ledValues[i].red << 8) | ((unsigned int)ledValues[i].green << 16) | ledValues[i].blue;
		//printBinary(colorBits, 24);
		//printf(" (binary, GRB order)\n");

		// Iterate through color bits to get wire bits
		for(int j=23; j>=0; j--) {
#ifdef WS2812_ASM_OPTI
			// Fetch word the bit is in
			unsigned int wordOffset = (int)(wireBit / 32);
			wireBit +=3;

			if (colorBits & (1 << j)) {
				PWMWaveform[wordOffset] |= startbitPattern;
			} else {
				PWMWaveform[wordOffset] = arm_Bit_Clear_imm(PWMWaveform[wordOffset], startbitPattern);
			}

			startbitPattern = arm_ror_imm(startbitPattern, 3);
#else
			unsigned char colorBit = (colorBits & (1 << j)) ? 1 : 0; // Holds current bit out of colorBits to be processed
			setPWMBit(wireBit, colorBit);
			wireBit +=3;
#endif
			/* old code for better understanding
			switch(colorBit) {
				case 1:
					//wireBits = 0b110;	// High, High, Low
					setPWMBit(wireBit++, 1);
					setPWMBit(wireBit++, 1);
					setPWMBit(wireBit++, 0);
					break;
				case 0:
					//wireBits = 0b100;	// High, Low, Low
					setPWMBit(wireBit++, 1);
					setPWMBit(wireBit++, 0);
					setPWMBit(wireBit++, 0);
					break;
			}*/
		}
	}

#ifdef WS2812_ASM_OPTI
	// calculate the bits manually since it is not needed with asm
	//wireBit += mLedCount * 24 *3;
	//printf(" %d\n", wireBit);
#endif
	//remove one to undo optimization
	wireBit --;

#ifdef WS2812_ASM_OPTI
	int rest = 32 - wireBit % 32; // 64: 32 - used Bits
	startbitPattern = (1 << (rest-1)); // set new bitpattern to start at the benigining of one bit (3 bit in wave form)
	rest += 32; // add one int extra for pwm

//	printBinary(startbitPattern, 32);
//	printf(" startbit\n");

	unsigned int oldwireBitValue = wireBit;
	unsigned int oldbitPattern = startbitPattern;

	// zero rest of the 4 bytes / int so that output is 0 (no data is send)
	for (int i = 0; i < rest; i += 3)
	{
		unsigned int wordOffset = (int)(wireBit / 32);
		wireBit += 3;
		PWMWaveform[wordOffset] = arm_Bit_Clear_imm(PWMWaveform[wordOffset], startbitPattern);
		startbitPattern = arm_ror_imm(startbitPattern, 3);
	}

#else
	// fill up the bytes
	int rest = 32 - wireBit % 32 + 32; // 64: 32 - used Bits + 32 (one int extra for pwm)
	unsigned int oldwireBitValue = wireBit;

	// zero rest of the 4 bytes / int so that output is 0 (no data is send)
	for (int i = 0; i < rest; i += 3)
	{
		setPWMBit(wireBit, 0);
		wireBit += 3;
	}
#endif

	memcpy ( ctl->sample, PWMWaveform, cbp->length );

	// Enable DMA and PWM engines, which should now send the data
	startTransfer();

	// restore bit pattern
	wireBit = oldwireBitValue;

#ifdef WS2812_ASM_OPTI
	startbitPattern = oldbitPattern;
	for (int i = 0; i < rest; i += 3)
	{
		unsigned int wordOffset = (int)(wireBit / 32);
		wireBit += 3;
		PWMWaveform[wordOffset] |= startbitPattern;
		startbitPattern = arm_ror_imm(startbitPattern, 3);
	}
#else
	for (int i = 0; i < rest; i += 3)
	{
		setPWMBit(wireBit, 1);
		wireBit += 3;
	}
#endif

#ifdef BENCHMARK
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeEnd);
	timespec result;

	result.tv_sec = timeEnd.tv_sec - timeStart.tv_sec;
	result.tv_nsec = timeEnd.tv_nsec - timeStart.tv_nsec;
	if (result.tv_nsec < 0)
	{
		result.tv_nsec = 1e9 - result.tv_nsec;
		result.tv_sec -= 1;
	}
	runCount ++;
	combinedNseconds += result.tv_nsec;
	shortestNseconds = result.tv_nsec < shortestNseconds ? result.tv_nsec : shortestNseconds;
#endif
	return 0;
}

int LedDeviceWS2812b::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}

LedDeviceWS2812b::~LedDeviceWS2812b()
{
	// Exit cleanly, freeing memory and stopping the DMA & PWM engines
	terminate(0);
#ifdef BENCHMARK
	printf("WS2812b Benchmark results: Runs %d - Avarage %lu (n) - Minimum %ld (n)\n",
			runCount, (runCount > 0 ? combinedNseconds / runCount : 0), shortestNseconds);
#endif
}


// =================================================================================================
//	  ________                                  .__
//	 /  _____/  ____   ____   ________________  |  |
//	/   \  ____/ __ \ /    \_/ __ \_  __ \__  \ |  |
//	\    \_\  \  ___/|   |  \  ___/|  | \// __ \|  |__
//	 \______  /\___  >___|  /\___  >__|  (____  /____/
//	        \/     \/     \/     \/           \/
// =================================================================================================

// Convenience functions
// --------------------------------------------------------------------------------------------------
// Print some bits of a binary number (2nd arg is how many bits)
void LedDeviceWS2812b::printBinary(unsigned int i, unsigned int bits)
{
	int x;
	for(x=bits-1; x>=0; x--)
	{
		printf("%d", (i & (1 << x)) ? 1 : 0);
		if(x % 16 == 0 && x > 0)
		{
			printf(" ");
		}
		else if(x % 4 == 0 && x > 0)
		{
			printf(":");
		}
	}
}

// Reverse the bits in a word
unsigned int reverseWord(unsigned int word)
{
	unsigned int output = 0;
	//unsigned char bit;
	int i;
	for(i=0; i<32; i++)
	{
		output |= word & (1 << i) ? 1 : 0;
		if(i<31)
		{
			output <<= 1;
		}
	}
	return output;
}

// Shutdown functions
// --------------------------------------------------------------------------------------------------
void LedDeviceWS2812b::terminate(int dummy) {
	// Shut down the DMA controller
	if(dma_reg)
	{
		CLRBIT(dma_reg[DMA_CS], DMA_CS_ACTIVE);
		usleep(100);
		SETBIT(dma_reg[DMA_CS], DMA_CS_RESET);
		usleep(100);
	}

	// Shut down PWM
	if(pwm_reg)
	{
		CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_PWEN1);
		usleep(100);
		pwm_reg[PWM_CTL] = (1 << PWM_CTL_CLRF1);
	}

	// Free the allocated memory
	if(page_map != 0)
	{
		free(page_map);
	}
}

void LedDeviceWS2812b::fatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	terminate(0);
}


// Memory management
// --------------------------------------------------------------------------------------------------
// Translate from virtual address to physical
unsigned int LedDeviceWS2812b::mem_virt_to_phys(void *virt)
{
	unsigned int offset = (uint8_t *)virt - virtbase;
	return page_map[offset >> PAGE_SHIFT].physaddr + (offset % PAGE_SIZE);
}

// Translate from physical address to virtual
unsigned int LedDeviceWS2812b::mem_phys_to_virt(uint32_t phys)
{
	unsigned int pg_offset = phys & (PAGE_SIZE - 1);
	unsigned int pg_addr = phys - pg_offset;

	for (unsigned int i = 0; i < NUM_PAGES; i++)
	{
		if (page_map[i].physaddr == pg_addr)
		{
			return (uint32_t)virtbase + i * PAGE_SIZE + pg_offset;
		}
	}
	fatal("Failed to reverse map phys addr %08x\n", phys);

	return 0;
}

// Map a peripheral's IO memory into our virtual memory, so we can read/write it directly
void * LedDeviceWS2812b::map_peripheral(uint32_t base, uint32_t len)
{
	int fd = open("/dev/mem", O_RDWR);
	void * vaddr;

	if (fd < 0)
	{
		fatal("Failed to open /dev/mem: %m\n");
	}
	vaddr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
	if (vaddr == MAP_FAILED)
	{
		fatal("Failed to map peripheral at 0x%08x: %m\n", base);
	}
	close(fd);

	return vaddr;
}

// Zero out the PWM waveform buffer
void LedDeviceWS2812b::clearPWMBuffer()
{
	memset(PWMWaveform, 0, NUM_DATA_WORDS * 4);	// Times four because memset deals in bytes.
}

// Set an individual bit in the PWM output array, accounting for word boundaries
// The (31 - bitIdx) is so that we write the data backwards, correcting its endianness
// This means getPWMBit will return something other than what was written, so it would be nice
// if the logic that calls this function would figure it out instead. (However, that's trickier)
void LedDeviceWS2812b::setPWMBit(unsigned int bitPos, unsigned char bit)
{
	// Fetch word the bit is in
	unsigned int wordOffset = (int)(bitPos / 32);
	unsigned int bitIdx = bitPos - (wordOffset * 32);

	switch(bit)
	{
		case 1:
			PWMWaveform[wordOffset] |= (1 << (31 - bitIdx));
			break;
		case 0:
			PWMWaveform[wordOffset] &= ~(1 << (31 - bitIdx));
			break;
	}
}

// ==== Init Hardware ====
void LedDeviceWS2812b::initHardware()
{
	int pid;
	int fd;
	char pagemap_fn[64];

	// Clear the PWM buffer
	// ---------------------------------------------------------------
	clearPWMBuffer();

	// Set up peripheral access
	// ---------------------------------------------------------------
	dma_reg = (unsigned int *) map_peripheral(DMA_BASE, DMA_LEN);
	dma_reg += 0x000;
	pwm_reg = (unsigned int *) map_peripheral(PWM_BASE, PWM_LEN);
	clk_reg = (unsigned int *) map_peripheral(CLK_BASE, CLK_LEN);
	gpio_reg = (unsigned int *) map_peripheral(GPIO_BASE, GPIO_LEN);


	// Set PWM alternate function for GPIO18
	// ---------------------------------------------------------------
	//gpio_reg[1] &= ~(7 << 24);
	//usleep(100);
	//gpio_reg[1] |= (2 << 24);
	//usleep(100);
	SET_GPIO_ALT(18, 5);


	// Allocate memory for the DMA control block & data to be sent
	// ---------------------------------------------------------------
	virtbase = (uint8_t *) mmap(
		NULL,													// Address
		NUM_PAGES * PAGE_SIZE,									// Length
		PROT_READ | PROT_WRITE,									// Protection
		MAP_SHARED |											// Shared
		MAP_ANONYMOUS |											// Not file-based, init contents to 0
		MAP_NORESERVE |											// Don't reserve swap space
		MAP_LOCKED,												// Lock in RAM (don't swap)
		-1,														// File descriptor
		0);														// Offset

	if (virtbase == MAP_FAILED)
	{
		fatal("Failed to mmap physical pages: %m\n");
		return;
	}

	if ((unsigned long)virtbase & (PAGE_SIZE-1))
	{
		fatal("Virtual address is not page aligned\n");
		return;
	}

	// Allocate page map (pointers to the control block(s) and data for each CB
	page_map = (page_map_t *) malloc(NUM_PAGES * sizeof(*page_map));
	if (page_map == 0)
	{
		fatal("Failed to malloc page_map: %m\n");
		return;
	}

	// Use /proc/self/pagemap to figure out the mapping between virtual and physical addresses
	pid = getpid();
	sprintf(pagemap_fn, "/proc/%d/pagemap", pid);
	fd = open(pagemap_fn, O_RDONLY);

	if (fd < 0)
	{
		fatal("Failed to open %s: %m\n", pagemap_fn);
	}

	off_t newOffset = (unsigned long)virtbase >> 9;
	if (lseek(fd, newOffset, SEEK_SET) != newOffset)
	{
		fatal("Failed to seek on %s: %m\n", pagemap_fn);
	}

	printf("Page map: %d pages\n", NUM_PAGES);
	for (unsigned int i = 0; i < NUM_PAGES; i++)
	{
		uint64_t pfn;
		page_map[i].virtaddr = virtbase + i * PAGE_SIZE;

		// Following line forces page to be allocated
		// (Note: Copied directly from Hirst's code... page_map[i].virtaddr[0] was just set...?)
		page_map[i].virtaddr[0] = 0;

		if (read(fd, &pfn, sizeof(pfn)) != sizeof(pfn)) {
			fatal("Failed to read %s: %m\n", pagemap_fn);
		}

		if (((pfn >> 55) & 0xfbf) != 0x10c) {  // pagemap bits: https://www.kernel.org/doc/Documentation/vm/pagemap.txt
			fatal("Page %d not present (pfn 0x%016llx)\n", i, pfn);
		}

		page_map[i].physaddr = (unsigned int)pfn << PAGE_SHIFT | 0x40000000;
		//printf("Page map #%2d: virtual %8p ==> physical 0x%08x [0x%016llx]\n", i, page_map[i].virtaddr, page_map[i].physaddr, pfn);
	}


	// Set up control block
	// ---------------------------------------------------------------
	struct control_data_s *ctl = (struct control_data_s *)virtbase;
	dma_cb_t *cbp = ctl->cb;
	// FIXME: Change this to use DEFINEs
	unsigned int phys_pwm_fifo_addr = 0x7e20c000 + 0x18;

	// No wide bursts, source increment, dest DREQ on line 5, wait for response, enable interrupt
	cbp->info = DMA_TI_CONFIGWORD;

	// Source is our allocated memory
	cbp->src = mem_virt_to_phys(ctl->sample);

	// Destination is the PWM controller
	cbp->dst = phys_pwm_fifo_addr;

	// 72 bits per pixel / 32 bits per word = 2.25 words per pixel
	// Add 1 to make sure the PWM FIFO gets the message: "we're sending zeroes"
	// Times 4 because DMA works in bytes, not words
	cbp->length = ((mLedCount * 2.25) + 1) * 4;
	if(cbp->length > NUM_DATA_WORDS * 4)
	{
		cbp->length = NUM_DATA_WORDS * 4;
	}

	// We don't use striding
	cbp->stride = 0;

	// These are reserved
	cbp->pad[0] = 0;
	cbp->pad[1] = 0;

	// Pointer to next block - 0 shuts down the DMA channel when transfer is complete
	cbp->next = 0;

	// Stop any existing DMA transfers
	// ---------------------------------------------------------------
	dma_reg[DMA_CS] |= (1 << DMA_CS_ABORT);
	usleep(100);
	dma_reg[DMA_CS] = (1 << DMA_CS_RESET);
	usleep(100);


	// PWM Clock
	// ---------------------------------------------------------------
	// Kill the clock
	// FIXME: Change this to use a DEFINE
	clk_reg[PWM_CLK_CNTL] = 0x5A000000 | (1 << 5);
	usleep(100);

	// Disable DMA requests
	CLRBIT(pwm_reg[PWM_DMAC], PWM_DMAC_ENAB);
	usleep(100);

	// The fractional part is quantized to a range of 0-1024, so multiply the decimal part by 1024.
	// E.g., 0.25 * 1024 = 256.
	// So, if you want a divisor of 400.5, set idiv to 400 and fdiv to 512.
	unsigned int idiv = 400;
	unsigned short fdiv = 0;	// Should be 16 bits, but the value must be <= 1024
	clk_reg[PWM_CLK_DIV] = 0x5A000000 | (idiv << 12) | fdiv;	// Set clock multiplier
	usleep(100);

	// Enable the clock. Next-to-last digit means "enable clock". Last digit is 1 (oscillator),
	// 4 (PLLA), 5 (PLLC), or 6 (PLLD) (according to the docs) although PLLA doesn't seem to work.
	// FIXME: Change this to use a DEFINE
	clk_reg[PWM_CLK_CNTL] = 0x5A000015;
	usleep(100);


	// PWM
	// ---------------------------------------------------------------
	// Clear any preexisting crap from the control & status register
	pwm_reg[PWM_CTL] = 0;

	// Set transmission range (32 bytes, or 1 word)
	// <32: Truncate. >32: Pad with SBIT1. As it happens, 32 is perfect.
	pwm_reg[PWM_RNG1] = 32;
	usleep(100);

	// Send DMA requests to fill the FIFO
	pwm_reg[PWM_DMAC] =
		(1 << PWM_DMAC_ENAB) |
		(8 << PWM_DMAC_PANIC) |
		(8 << PWM_DMAC_DREQ);
	usleep(1000);

	// Clear the FIFO
	SETBIT(pwm_reg[PWM_CTL], PWM_CTL_CLRF1);
	usleep(100);

	// Don't repeat last FIFO contents if it runs dry
	CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_RPTL1);
	usleep(100);

	// Silence (default) bit is 0
	CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_SBIT1);
	usleep(100);

	// Polarity = default (low = 0, high = 1)
	CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_POLA1);
	usleep(100);

	// Enable serializer mode
	SETBIT(pwm_reg[PWM_CTL], PWM_CTL_MODE1);
	usleep(100);

	// Use FIFO rather than DAT1
	SETBIT(pwm_reg[PWM_CTL], PWM_CTL_USEF1);
	usleep(100);

	// Disable MSEN1
	CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_MSEN1);
	usleep(100);


	// DMA
	// ---------------------------------------------------------------
	// Raise an interrupt when transfer is complete, which will set the INT flag in the CS register
	SETBIT(dma_reg[DMA_CS], DMA_CS_INT);
	usleep(100);

	// Clear the END flag (by setting it - this is a "write 1 to clear", or W1C, bit)
	SETBIT(dma_reg[DMA_CS], DMA_CS_END);
	usleep(100);

	// Send the physical address of the control block into the DMA controller
	dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(ctl->cb);
	usleep(100);

	// Clear error flags, if any (these are also W1C bits)
	// FIXME: Use a define instead of this
	dma_reg[DMA_DEBUG] = 7;
	usleep(100);
}

// Begin the transfer
void LedDeviceWS2812b::startTransfer()
{
	// Enable DMA
	dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(((struct control_data_s *) virtbase)->cb);
	dma_reg[DMA_CS] = DMA_CS_CONFIGWORD | (1 << DMA_CS_ACTIVE);
	usleep(100);

	// Enable PWM
	SETBIT(pwm_reg[PWM_CTL], PWM_CTL_PWEN1);
}
