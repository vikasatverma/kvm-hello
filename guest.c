#include <stddef.h>
#include <stdint.h>
#include <string.h>
struct file
{
	int fd;
	char *filename;
	char *mode;
	int state;
};



static void outb(uint16_t port, uint8_t value)
{
	asm("outb %0,%1"
		: /* empty */
		: "a"(value), "Nd"(port)
		: "memory");
}

static inline uint32_t inb(uint16_t port)
{
	uint8_t ret;
	asm("in %1, %0"
		: "=a"(ret)
		: "Nd"(port)
		: "memory");
	return ret;
}


static void printVal(uint32_t val)
{
	outb(0xF1, val);
}

static uint32_t getNumExits()
{
	uint32_t exit_count;
	exit_count = inb(0xF2);
	return exit_count;
}

static void display(const char *str)
{
	uint32_t stringPtr = (intptr_t)str;
	outb(0xF3, stringPtr);
}


static uint32_t fopen(char *filename, char* mode){
		outb(0xF4,(intptr_t)filename);
		outb(0xF4,(intptr_t)mode);
		uint32_t fd ;
		fd = *(long *)0x400;
		outb(0xF1,fd);
		return fd;
		
}

static void fwrite(char *data, uint32_t fd){
		outb(0xF5,(intptr_t)data);
		outb(0xF5,fd);

}



static char* fread(char *ptr, uint32_t size, uint32_t fd)
{
	outb(0xF6,size);
	outb(0xF6,fd);
	ptr = (char *)(intptr_t)inb(0xF6);
	return ptr;
}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{


	char *filename="writefile.txt";
	char *mode="w";
	uint32_t fd=0;
	fd=fopen(filename,mode);
	char *filecontent="this is some content to be sent to the file with fd 0";
	fwrite(filecontent,fd);
	filename = "asdfasfsafda";
	mode="w";
	uint32_t newfd=fopen(filename,mode);
	printVal(newfd);
	filecontent="this is some content to be sent to the file with fd 1";
	fwrite(filecontent,fd);
	newfd=fopen("test.c","r");
	char content[50];
	fread(content,5,newfd);





/*
	const char *p;

	for (p = "Hello, world!\n"; *p; ++p)
		outb(0xE9, *p);
	// 14 Exits

	printVal((uint32_t)30);
	// 1 Exit.......... Total exits: 15

	uint32_t numExits = getNumExits();
	printVal(numExits);
	//2 Exits ie one to ask for count and one to print the count...Total exits: 18

	const char *str = "This is a random string";
	display(str);
	//1 exit

	numExits = getNumExits();
	printVal(numExits);

	// fopen("filename","w");

*/


	*(long *)0x400 = 42;

	for (;;)
		asm("hlt"
			: /* empty */
			: "a"(42)
			: "memory");
}
