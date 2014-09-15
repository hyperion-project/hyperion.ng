// For license and other informations see LedDeviceWS2812s.h
// To activate: use led device "ws2812s" in the hyperion configuration

// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>

// Linux includes
#include <fcntl.h>
//#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceWS2812s.h"

LedDeviceWS2812s::LedDeviceWS2812s() :
	LedDevice(),
	mLedCount(0)
{
	// Init PWM generator and clear LED buffer
	initHardware();
	//clearLEDBuffer();
}


int LedDeviceWS2812s::write(const std::vector<ColorRgb> &ledValues)
{
	mLedCount = ledValues.size();
	//printf("Set leds, number: %d\n", mLedCount);

//	const unsigned dataLen = ledValues.size() * sizeof(ColorRgb);
//	const uint8_t * dataPtr = reinterpret_cast<const uint8_t *>(ledValues.data());

	// Clear out the PWM buffer
	// Disabled, because we will overwrite the buffer anyway.

	// Read data from LEDBuffer[], translate it into wire format, and write to PWMWaveform
//	unsigned int LEDBuffeWordPos = 0;
//	unsigned int PWMWaveformBitPos = 0;
	unsigned int colorBits = 0;			// Holds the GRB color before conversion to wire bit pattern
	unsigned char colorBit = 0;			// Holds current bit out of colorBits to be processed
	unsigned int wireBit = 0;			// Holds the current bit we will set in PWMWaveform
//	Color_t color;

	for(size_t i=0; i<mLedCount; i++) {
		// Create bits necessary to represent one color triplet (in GRB, not RGB, order)
		//printf("RGB: %d, %d, %d\n", ledValues[i].red, ledValues[i].green, ledValues[i].blue);
		colorBits = ((unsigned int)ledValues[i].red << 8) | ((unsigned int)ledValues[i].green << 16) | ledValues[i].blue;
		//printBinary(colorBits, 24);
		//printf(" (binary, GRB order)\n");

		// Iterate through color bits to get wire bits
		for(int j=23; j>=0; j--) {
			colorBit = (colorBits & (1 << j)) ? 1 : 0;
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
			}
		}
	}

	// Copy PWM waveform to DMA's data buffer
	//printf("Copying %d words to DMA data buffer\n", NUM_DATA_WORDS);
	ctl = (struct control_data_s *)virtbase;
	dma_cb_t *cbp = ctl->cb;

	// 72 bits per pixel / 32 bits per word = 2.25 words per pixel
	// Add 1 to make sure the PWM FIFO gets the message: "we're sending zeroes"
	// Times 4 because DMA works in bytes, not words
	cbp->length = ((mLedCount * 2.25) + 1) * 4;
	if(cbp->length > NUM_DATA_WORDS * 4) {
		cbp->length = NUM_DATA_WORDS * 4;
	}

	// This block is a major CPU hog when there are lots of pixels to be transmitted.
	// It would go quicker with DMA.
	for(unsigned int i = 0; i < (cbp->length / 4); i++) {
		ctl->sample[i] = PWMWaveform[i];
	}


	// Enable DMA and PWM engines, which should now send the data
	startTransfer();

	// Wait long enough for the DMA transfer to finish
	// 3 RAM bits per wire bit, so 72 bits to send one color command.
	//float bitTimeUSec = (float)(NUM_DATA_WORDS * 32) * 0.4;	// Bits sent * time to transmit one bit, which is 0.4μSec
	//printf("Delay for %d μSec\n", (int)bitTimeUSec);
	//usleep((int)bitTimeUSec);

	return 0;
}

int LedDeviceWS2812s::switchOff()
{
	return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
}

LedDeviceWS2812s::~LedDeviceWS2812s()
{
	// Exit cleanly, freeing memory and stopping the DMA & PWM engines
		// We trap all signals (including Ctrl+C), so even if you don't get here, it terminates correctly
		terminate(0);
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
void LedDeviceWS2812s::printBinary(unsigned int i, unsigned int bits) {
	int x;
	for(x=bits-1; x>=0; x--) {
		printf("%d", (i & (1 << x)) ? 1 : 0);
		if(x % 16 == 0 && x > 0) {
			printf(" ");
		} else if(x % 4 == 0 && x > 0) {
			printf(":");
		}
	}
}

// Reverse the bits in a word
unsigned int reverseWord(unsigned int word) {
	unsigned int output = 0;
	//unsigned char bit;
	int i;
	for(i=0; i<32; i++) {
		//bit = word & (1 << i) ? 1 : 0;
		output |= word & (1 << i) ? 1 : 0;
		if(i<31) {
			output <<= 1;
		}
	}
	return output;
}

// Not sure how this is better than usleep...?
/*
static void udelay(int us) {
	struct timespec ts = { 0, us * 1000 };
	nanosleep(&ts, NULL);
}
*/


// Shutdown functions
// --------------------------------------------------------------------------------------------------
void LedDeviceWS2812s::terminate(int dummy) {
	// Shut down the DMA controller
	if(dma_reg) {
		CLRBIT(dma_reg[DMA_CS], DMA_CS_ACTIVE);
		usleep(100);
		SETBIT(dma_reg[DMA_CS], DMA_CS_RESET);
		usleep(100);
	}

	// Shut down PWM
	if(pwm_reg) {
		CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_PWEN1);
		usleep(100);
		pwm_reg[PWM_CTL] = (1 << PWM_CTL_CLRF1);
	}

	// Free the allocated memory
	if(page_map != 0) {
		free(page_map);
	}

	//exit(1);
}

void LedDeviceWS2812s::fatal(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	terminate(0);
}


// Memory management
// --------------------------------------------------------------------------------------------------
// Translate from virtual address to physical
unsigned int LedDeviceWS2812s::mem_virt_to_phys(void *virt) {
	unsigned int offset = (uint8_t *)virt - virtbase;
	return page_map[offset >> PAGE_SHIFT].physaddr + (offset % PAGE_SIZE);
}

// Translate from physical address to virtual
unsigned int LedDeviceWS2812s::mem_phys_to_virt(uint32_t phys) {
	unsigned int pg_offset = phys & (PAGE_SIZE - 1);
	unsigned int pg_addr = phys - pg_offset;

	for (unsigned int i = 0; i < NUM_PAGES; i++) {
		if (page_map[i].physaddr == pg_addr) {
			return (uint32_t)virtbase + i * PAGE_SIZE + pg_offset;
		}
	}
	fatal("Failed to reverse map phys addr %08x\n", phys);

	return 0;
}

// Map a peripheral's IO memory into our virtual memory, so we can read/write it directly
void * LedDeviceWS2812s::map_peripheral(uint32_t base, uint32_t len) {
	int fd = open("/dev/mem", O_RDWR);
	void * vaddr;

	if (fd < 0)
		fatal("Failed to open /dev/mem: %m\n");
	vaddr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
	if (vaddr == MAP_FAILED)
		fatal("Failed to map peripheral at 0x%08x: %m\n", base);
	close(fd);

	return vaddr;
}

// Zero out the PWM waveform buffer
void LedDeviceWS2812s::clearPWMBuffer() {
	memset(PWMWaveform, 0, NUM_DATA_WORDS * 4);	// Times four because memset deals in bytes.
}

// Set an individual bit in the PWM output array, accounting for word boundaries
// The (31 - bitIdx) is so that we write the data backwards, correcting its endianness
// This means getPWMBit will return something other than what was written, so it would be nice
// if the logic that calls this function would figure it out instead. (However, that's trickier)
void LedDeviceWS2812s::setPWMBit(unsigned int bitPos, unsigned char bit) {

	// Fetch word the bit is in
	unsigned int wordOffset = (int)(bitPos / 32);
	unsigned int bitIdx = bitPos - (wordOffset * 32);

	//printf("bitPos=%d wordOffset=%d bitIdx=%d value=%d\n", bitPos, wordOffset, bitIdx, bit);

	switch(bit) {
		case 1:
			PWMWaveform[wordOffset] |= (1 << (31 - bitIdx));
//			PWMWaveform[wordOffset] |= (1 << bitIdx);
			break;
		case 0:
			PWMWaveform[wordOffset] &= ~(1 << (31 - bitIdx));
//			PWMWaveform[wordOffset] &= ~(1 << bitIdx);
			break;
	}
}

// =================================================================================================
//	.___       .__  __      ___ ___                  .___
//	|   | ____ |__|/  |_   /   |   \_____ _______  __| _/_  _  _______ _______   ____
//	|   |/    \|  \   __\ /    ~    \__  \\_  __ \/ __ |\ \/ \/ /\__  \\_  __ \_/ __ \
//	|   |   |  \  ||  |   \    Y    // __ \|  | \/ /_/ | \     /  / __ \|  | \/\  ___/
//	|___|___|  /__||__|    \___|_  /(____  /__|  \____ |  \/\_/  (____  /__|    \___  >
//	         \/                  \/      \/           \/              \/            \/
// =================================================================================================

void LedDeviceWS2812s::initHardware() {
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

	if (virtbase == MAP_FAILED) {
		fatal("Failed to mmap physical pages: %m\n");
	}

	if ((unsigned long)virtbase & (PAGE_SIZE-1)) {
		fatal("Virtual address is not page aligned\n");
	}

	//printf("virtbase mapped 0x%x bytes at 0x%x\n", NUM_PAGES * PAGE_SIZE, virtbase);

	// Allocate page map (pointers to the control block(s) and data for each CB
	page_map = (page_map_t *) malloc(NUM_PAGES * sizeof(*page_map));
	if (page_map == 0) {
		fatal("Failed to malloc page_map: %m\n");
	} else {
		//printf("Allocated 0x%x bytes for page_map at 0x%x\n", NUM_PAGES * sizeof(*page_map), page_map);
	}

	// Use /proc/self/pagemap to figure out the mapping between virtual and physical addresses
	pid = getpid();
	sprintf(pagemap_fn, "/proc/%d/pagemap", pid);
	fd = open(pagemap_fn, O_RDONLY);

	if (fd < 0) {
		fatal("Failed to open %s: %m\n", pagemap_fn);
	}

	if (lseek(fd, (unsigned long)virtbase >> 9, SEEK_SET) != (unsigned long)virtbase >> 9) {
		fatal("Failed to seek on %s: %m\n", pagemap_fn);
	}

	printf("Page map: %d pages\n", NUM_PAGES);
	for (unsigned int i = 0; i < NUM_PAGES; i++) {
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
	ctl = (struct control_data_s *)virtbase;
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
	if(cbp->length > NUM_DATA_WORDS * 4) {
		cbp->length = NUM_DATA_WORDS * 4;
	}

	// We don't use striding
	cbp->stride = 0;

	// These are reserved
	cbp->pad[0] = 0;
	cbp->pad[1] = 0;

	// Pointer to next block - 0 shuts down the DMA channel when transfer is complete
	cbp->next = 0;

	// Testing
	/*
	ctl = (struct control_data_s *)virtbase;
	ctl->sample[0] = 0x00000000;
	ctl->sample[1] = 0x000000FA;
	ctl->sample[2] = 0x0000FFFF;
	ctl->sample[3] = 0xAAAAAAAA;
	ctl->sample[4] = 0xF0F0F0F0;
	ctl->sample[5] = 0x0A0A0A0A;
	ctl->sample[6] = 0xF00F0000;
	*/


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
void LedDeviceWS2812s::startTransfer() {
	// Enable DMA
	dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(ctl->cb);
	dma_reg[DMA_CS] = DMA_CS_CONFIGWORD | (1 << DMA_CS_ACTIVE);
	usleep(100);

	// Enable PWM
	SETBIT(pwm_reg[PWM_CTL], PWM_CTL_PWEN1);

//	dumpPWM();
//	dumpDMA();
}
