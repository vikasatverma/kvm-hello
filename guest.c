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
	uint32_t fd = *(long *)0x400;
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

static void fclose(uint32_t fd){
	outb(0xF7,fd);
}

static void fseek(uint32_t fd, uint32_t offset, uint32_t whence){
	outb(0xF8,fd);
	outb(0xF8,offset);
	outb(0xF8,whence);

}

void
	__attribute__((noreturn))
	__attribute__((section(".start")))
	_start(void)
{

//Part 1
		printVal((uint32_t)30);
		// 1 Exit.......... Total exits: 15

		uint32_t numExits = getNumExits();
		printVal(numExits);
		//2 Exits ie one to ask for count and one to print the count...Total exits: 18

		// const char *str = "This is a random string";
		display("This is a random string");
		//1 exit

		numExits = getNumExits();
		printVal(numExits);

//Part 2

		uint32_t fd1=fopen("file_1.txt","w");

		fwrite("This course covers basic and advanced concepts related to the virtualization and how it enables the latest revolution of cloud computing. You will understand the various virtualization paradigms of full virtualization, para virtualization, and hardware-assisted virtualization, for CPU, memory, and I/O. You will also learn about containers and related concepts. The later part of the course will cover various advanced topics related to virtual machine migration, provisioning, nested virtualization, security, unikernels, and so on. The programming assignments in the course are designed to reinforce the concepts learnt in class.",fd1);


//Part 3

		char buffer[100];
		uint32_t fd2;


		fd2 = fopen("kvm-hello-world.c","r");
		fseek(fd2,50,0);
		fread(buffer,100,fd2);


		fclose(fd1);
		fclose(fd2);






	*(long *)0x400 = 42;

	for (;;)
		asm("hlt"
			: /* empty */
			: "a"(42)
			: "memory");
}
