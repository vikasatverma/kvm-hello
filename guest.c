#include <stddef.h>
#include <stdint.h>

static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}


static inline uint32_t inb(uint16_t port) {
  uint8_t ret;
  asm("in %1, %0" : "=a"(ret) : "Nd"(port) : "memory" );
  return ret;
}

static void printVal(uint32_t val){
	outb(0xF1,val);
}


static uint32_t getNumExits( ){
	uint32_t exit_count;
	exit_count=inb(0xF2);
	return exit_count;
}


static void display(const char *str){
	
}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	const char *p;

	for (p = "Hello, world!\n"; *p; ++p)
		outb(0xE9, *p);

	// uint32_t g=0;
	// g=inb(0xE0);
	// *(long *) 0x400 = g;


	printVal((uint32_t)-1);


	uint32_t numExits = getNumExits();
	printVal(numExits);

	const char *str = "This is a random string";
	display(str);

	*(long *) 0x400 = 42;


	for (;;)
		asm("hlt" : /* empty */ : "a" (42) : "memory");
}
