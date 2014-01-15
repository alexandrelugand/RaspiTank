//
// Heng Long Tiger I tank controller for Raspberry Pi
// Ian Renton, June 2012
// http://ianrenton.com
// 
// Based on the GPIO example by Dom and Gert
// (http://elinux.org/Rpi_Low-level_peripherals#GPIO_Driving_Example_.28C.29)
// Using Heng Long op codes discovered by ancvi_pIII
// (http://www.rctanksaustralia.com/forum/viewtopic.php?p=1314#p1314)
//

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

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
#include <iostream>
#include <string>

using namespace std;

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
char *gpio_mem, *gpio_map;
char *spi0_mem, *spi0_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

// N.B. These have been reversed compared to Gert & Dom's original code!
// This is because the transistor board I use for talking to the Heng
// Long RX18 inverts the signal.  So the GPIO_SET pointer here actually
// sets the GPIO pin low - but that ends up as a high at the tank.
#define GPIO_CLR *(gpio+7)  // sets   bits which are 1, ignores bits which are 0
#define GPIO_SET *(gpio+10) // clears bits which are 1, ignores bits which are 0

// GPIO pin that connects to the Heng Long main board
// (Pin 7 is the top right pin on the Pi's GPIO, next to the yellow video-out)
//#define PIN 7
#define PIN 14

// Heng Long tank bit-codes
int idle = 0xFE40121C;
int ignition = 0xFE401294;
//int neutral = 0xFE3C0F00;
int neutral = 0xFE380F00;
int left_slow = 0xFE400608;
int left_fast = 0xFE400010;
int right_slow = 0xFE401930;
int right_fast = 0xFE401E2C;
//int fwd_slow = 0xFE200F34;
int fwd_slow = 0xFE280F14;
int fwd_fast = 0xFE000F3C;
//int rev_slow = 0xFE580F08;
int rev_slow = 0xFE500F28;
int rev_fast = 0xFE780F00;
int turret_left = 0xFE408F0C;
int turret_right = 0xFE410F28;
int turret_elev = 0xFE404F3C;
int fire = 0xFE442F34;
int machine_gun = 0xFE440F78;
int recoil = 0xFE420F24;
int frameInt = 8; //8ms

// Function declarations
void setup_io();
void sendCode(int code);
void sendBit(int bit);
void StartEngine();
void StopEngine();
void CRC(int& cmd);

// Main
int main(int argc, char **argv) {

	int g, rep, i;

	// Set up gpio pointer for direct register access
	setup_io();

	// Switch the relevant GPIO pin to output mode
	INP_GPIO(PIN); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(PIN);

	GPIO_CLR = 1 << PIN;

	StartEngine();

	// Loop, sending movement commands indefinitely
	for(;;)
	{		
		string strCmd, strCount;
		int cmd = 0, count = 0;

		printf("Enter command:\n");
		cin >> strCmd;
		if (strCmd.compare("exit") == 0)
			break;
		cmd = strtoul(strCmd.c_str(), NULL, 16);
		CRC(cmd);
		printf("Cmd: 0x%08X\n", cmd);

		printf("Enter frame count:\n");
		cin >> strCount;
		count = strtoul(strCount.c_str(), NULL, 10);
						
		for (int j = 0; j<count; j++)
			sendCode(cmd);
	}

	StopEngine();

	return 0;

} // main

void CRC(int& cmd)
{
	unsigned char crc = 0x0;

	//XOR bits à bits par mot de 4 bits;
	int word = cmd & 0x00F00000;
	crc ^= word >> 20;

	word = cmd & 0x000F0000;
	crc ^= word >> 16;

	word = cmd & 0x0000F000;
	crc ^= word >> 12;

	word = cmd & 0x00000F00;
	crc ^= word >> 8;

	//Le dernier groupe de 4 bits est constitué de deux derniers bits de la trame en poids faible
	word = cmd & 0x000000C0;
	crc ^= word >> 6;

	//Replacement sur les bits 2 à 5 de la trame
	crc = crc << 2;

	//Place le CRC dans la trame
	cmd = cmd | crc;
}

void StartEngine()
{
	// Send the idle and ignition codes
	printf("Idle\n");

	for (int i = 0; i<40; i++)
	{
		sendCode(idle);
	}
	
	printf("Ignition\n");
	for (int i = 0; i<40; i++)
	{
		sendCode(ignition);
	}
	
	printf("Waiting for ignition...\n");
	for (int i = 0; i<600; i++)
	{
		sendCode(idle);
	}
	
	printf("Ignition done\n");   //  <-- valeur i déterminant pour assure le démarrage
	for (int i = 0; i<9; i++)
	{
		sendCode(ignition);
	}

	printf("Warm up\n");
	for (int i = 0; i<100; i++)
	{
		sendCode(neutral);
	}	
	
	printf("Engine started\n");
}

void StopEngine()
{
	// Send the ignition and idle codes
	printf("Stopping engine...\n");	
	for (int i = 0; i < 15; i++)
	{
		sendCode(ignition);
	}

	for (int i = 0; i < 40; i++)
	{
		sendCode(idle);
	}
	printf("Engine stopped\n");
}


// Sends one individual code to the main tank controller
void sendCode(int code) {
	// Send header "bit" (not a valid Manchester code)
	GPIO_SET = 1 << PIN;
	usleep(600);

	// Send the code itself, bit by bit using Manchester coding
	int i;
	for (i = 0; i<32; i++) {
		int bit = (code >> (31 - i)) & 0x1;
		sendBit(bit);
	}

	// Force a 4ms gap between messages
	GPIO_CLR = 1 << PIN;
	//usleep(4000);

	usleep(frameInt*1000);
} // sendCode


// Sends one individual bit using Manchester coding
// 1 = high-low, 0 = low-high
void sendBit(int bit) {
	//printf("%d", bit);

	if (bit == 1) {
		GPIO_SET = 1 << PIN;
		usleep(300);
		GPIO_CLR = 1 << PIN;
		usleep(300);
	}
	else {
		GPIO_CLR = 1 << PIN;
		usleep(300);
		GPIO_SET = 1 << PIN;
		usleep(300);
	}
} // sendBit


// Set up a memory region to access GPIO
void setup_io() {

	/* open /dev/mem */
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		printf("can't open /dev/mem \n");
		exit(-1);
	}

	/* mmap GPIO */

	// Allocate MAP block
	if ((gpio_mem = (char*)malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL) {
		printf("allocation error \n");
		exit(-1);
	}

	// Make sure pointer is on 4K boundary
	if ((unsigned long)gpio_mem % PAGE_SIZE)
		gpio_mem += PAGE_SIZE - ((unsigned long)gpio_mem % PAGE_SIZE);

	// Now map it
	gpio_map = (char *)mmap(
		(caddr_t)gpio_mem,
		BLOCK_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_FIXED,
		mem_fd,
		GPIO_BASE
		);

	if ((long)gpio_map < 0) {
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}

	// Always use volatile pointer!
	gpio = (volatile unsigned *)gpio_map;

} // setup_io
