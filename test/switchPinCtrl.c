//
// Simple byte wise SPI driver
// Demo how to set up memmap and access SPI registers.
// Code seems to be working but has not been much tested.
// G.J. van Loo 15-Jan-2012
//



// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define UART0_BASE               (BCM2708_PERI_BASE + 0x201000) /* Uart 0 */
#define UART1_BASE               (BCM2708_PERI_BASE + 0x215000) /* Uart 1 */
#define MCORE_BASE               (BCM2708_PERI_BASE + 0x0000)   /* Fake frame buffer device */
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define SPI0_BASE                (BCM2708_PERI_BASE + 0x204000) /* SPI0 controller */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
char *gpio_mem, *gpio_map;
char *spi0_mem, *spi0_map;


// I/O access
volatile unsigned *gpio;
volatile unsigned *spi0;


// SPI operation

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

//
#define SPI0_CNTLSTAT *(spi0 + 0)
#define SPI0_FIFO     *(spi0 + 1)
#define SPI0_CLKSPEED *(spi0 + 2)

// SPI0_CNTLSTAT register bits

#define SPI0_CS_CS2ACTHIGH   0x00800000 // CS2 active high
#define SPI0_CS_CS1ACTHIGH   0x00400000 // CS1 active high
#define SPI0_CS_CS0ACTHIGH   0x00200000 // CS0 active high
#define SPI0_CS_RXFIFOFULL   0x00100000 // Receive FIFO full
#define SPI0_CS_RXFIFO3_4    0x00080000 // Receive FIFO 3/4 full
#define SPI0_CS_TXFIFOSPCE   0x00040000 // Transmit FIFO has space
#define SPI0_CS_RXFIFODATA   0x00020000 // Receive FIFO has data
#define SPI0_CS_DONE         0x00010000 // SPI transfer done. WRT to CLR!
#define SPI0_CS_MOSI_INPUT   0x00001000 // MOSI is input, read from MOSI (BI-dir mode)
#define SPI0_CS_DEASRT_CS    0x00000800 // De-assert CS at end
#define SPI0_CS_RX_IRQ       0x00000400 // Receive irq enable
#define SPI0_CS_DONE_IRQ     0x00000200 // irq when done
#define SPI0_CS_DMA_ENABLE   0x00000100 // Run in DMA mode
#define SPI0_CS_ACTIVATE     0x00000080 // Activate: be high before starting
#define SPI0_CS_CS_POLARIT   0x00000040 // Chip selects active high
#define SPI0_CS_CLRTXFIFO    0x00000020 // Clear TX FIFO    (auto clear bit)
#define SPI0_CS_CLRRXFIFO    0x00000010 // Clear RX FIFO    (auto clear bit)
#define SPI0_CS_CLRFIFOS     0x00000030 // Clear BOTH FIFOs (auto clear bit)
#define SPI0_CS_CLK_IDLHI    0x00000008 // Clock pin is high when idle
#define SPI0_CS_CLKTRANS     0x00000004 // 0=first clock in middle of data bit
                                        // 1=first clock at begin of data bit
#define SPI0_CS_CHIPSEL0     0x00000000 // Use chip select 0
#define SPI0_CS_CHIPSEL1     0x00000001 // Use chip select 1
#define SPI0_CS_CHIPSEL2     0x00000002 // Use chip select 2
#define SPI0_CS_CHIPSELN     0x00000003 // No chip select (e.g. use GPIO pin)

#define SPI0_CS_CLRALL      (SPI0_CS_CLRFIFOS|SPI0_CS_DONE)

#define ISASC(x) ((x)>=0x20 && (x)<=0x7F)

void setup_io();

int main(int argc, char **argv)
{ int g;

  setup_io();  // Set up direct access to I/O for GPIO and SPI

  // Switch GPIO 7..11 to SPI mode (ALT function 0)

 /************************************************************************\
  * You are about to change the GPIO settings of your computer.          *
  * Mess this up and it will stop working!                               *
  * It might be a good idea to 'sync' before running this program        *
  * so at least you still have your code changes written to the SD-card! *
 \************************************************************************/

  for (g=7; g<=11; g++)
  {
    INP_GPIO(g);       // clear bits (= input)
    SET_GPIO_ALT(g,0); // set function 0
  }

  return 0;

} // main


//
// Set up a memory regions to access GPIO and SPI0
//
void setup_io()
{

   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit (-1);
   }

   /* mmap GPIO */
   if ((gpio_mem = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
      printf("allocation error \n");
      exit (-1);
   }
   if ((unsigned long)gpio_mem % PAGE_SIZE)
     gpio_mem += PAGE_SIZE - ((unsigned long)gpio_mem % PAGE_SIZE);

   gpio_map = (unsigned char *)mmap(
      (caddr_t)gpio_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      GPIO_BASE
   );

   if ((long)gpio_map < 0) {
      printf("mmap error %ld\n", (long)gpio_map);
      exit (-1);
   }
   gpio = (volatile unsigned *)gpio_map;

   /* mmap SPI0 */
   if ((spi0_mem = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
      printf("allocation error \n");
      exit (-1);
   }
   if ((unsigned long)spi0_mem % PAGE_SIZE)
     spi0_mem += PAGE_SIZE - ((unsigned long)spi0_mem % PAGE_SIZE);

   spi0_map = (unsigned char *)mmap(
      (caddr_t)spi0_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      SPI0_BASE
   );


   printf("SPI mapped from 0x%d to 0x%p\n",SPI0_BASE,spi0_map);

   if ((long)spi0_map < 0) {
      printf("mmap error %ld\n", (long)spi0_map);
      exit (-1);
   }
   spi0 = (volatile unsigned *)spi0_map;

} // setup_io
