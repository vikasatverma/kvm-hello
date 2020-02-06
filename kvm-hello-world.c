#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>

#define viva 1
#define VMSleep 0
#define switchcase 1
#define detail 0
/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

struct vm
{
	int sys_fd;
	int fd;
	char *mem;
};

// struct ftype
// {
// 	int fd;
// 	char *filename;
// 	char *mode;
// 	int state;
// };
/*
	vm_init(&vm, 0x200000);
*/


int curIndex=0;
FILE *FILEarray[50]={};
void vm_init(struct vm *vm, size_t mem_size)
{
	if (viva)
		printf("vm_init %d\n", __LINE__);

	int api_ver;
	struct kvm_userspace_memory_region userspace_mem_region;

	// Open kvm device file in read/write mode
	vm->sys_fd = open("/dev/kvm", O_RDWR);

	if (vm->sys_fd < 0)
	{
		perror("open /dev/kvm");
		exit(1);
	}

	api_ver = ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
	if (api_ver < 0)
	{
		perror("KVM_GET_API_VERSION");
		exit(1);
	}

	if (api_ver != KVM_API_VERSION)
	{
		fprintf(stderr, "Got KVM api version %d, expected %d\n",
				api_ver, KVM_API_VERSION);
		exit(1);
	}

	// Create a VM and store in vm data struct
	vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);
	if (vm->fd < 0)
	{
		perror("KVM_CREATE_VM");
		exit(1);
	}

	/*
		This ioctl defines the physical address of a three-page region in the guest
		physical address space.  The region must be within the first 4GB of the
		guest physical address space and must not conflict with any memory slot
		or any mmio address.  The guest may malfunction if it accesses this memory
		region.
	*/

	if (ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) //3GB
	{
		perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);

	if (vm->mem == MAP_FAILED)
	{
		perror("mmap mem");
		exit(1);
	}

	/* 
		used to give advice or directions to the kernel about the address 
		range beginning at address vm->mem and with size mem_size bytes
	*/
	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	userspace_mem_region.slot = 0;
	userspace_mem_region.flags = 0;
	userspace_mem_region.guest_phys_addr = 0;
	userspace_mem_region.memory_size = mem_size;
	userspace_mem_region.userspace_addr = (unsigned long)vm->mem;

	/*
ps -ax | grep kvm
pmap -p <pid>
*/
	if (viva)
		printf("Allocating %lu MB of physical memory to guest OS at %lx"
			   " in virtual address space of host at line: %d\n",
			   mem_size / 1024 / 1024, (unsigned long)vm->mem, __LINE__ + 1);
	if (ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &userspace_mem_region) < 0)
	{
		perror("KVM_SET_USER_MEMORY_REGION");
		exit(1);
	}
}

struct vcpu
{
	int fd;
	struct kvm_run *kvm_run;
};

void vcpu_init(struct vm *vm, struct vcpu *vcpu)
{
	if (viva)

		printf("vcpu_init %d\n", __LINE__);

	int vcpu_mmap_size;

	vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);
	if (vcpu->fd < 0)
	{
		perror("KVM_CREATE_VCPU");
		exit(1);
	}

	vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if (vcpu_mmap_size <= 0)
	{
		perror("KVM_GET_VCPU_MMAP_SIZE");
		exit(1);
	}

	vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
						 MAP_SHARED, vcpu->fd, 0);
	if (viva)
		printf("Mapping %u KB as VCPU runtime memory located at %lx in"
			   " virtual address space of hypervisor in line: %d\n",
			   vcpu_mmap_size / 1024, (long int)vcpu->kvm_run, __LINE__ - 5);
	if (vcpu->kvm_run == MAP_FAILED)
	{
		perror("mmap kvm_run");
		exit(1);
	}
}


int run_vm(struct vm *vm, struct vcpu *vcpu, size_t sz)
{
	if (viva)

		printf("run_vm %d\n", __LINE__);

	struct kvm_regs regs;
	uint64_t memval = 0;
// Starting from 1 to account for the exit happened to print the value as the value received 
// after fetching will have become old.
	for (uint32_t exit_count = 1;;)
	{

		if (viva && detail)
			printf("Shifting the control to guest OS in line %d\n", __LINE__ + 1);
		if (ioctl(vcpu->fd, KVM_RUN, 0) < 0)
		{
			perror("KVM_RUN");
			exit(1);
		}
		exit_count++;
		if (viva && detail)
			printf("Shifting the control back to hypervisor in line %d\n", __LINE__ - 7);

		if (viva && switchcase)
		{
			printf("Exit reason: %u on port %x ie ", vcpu->kvm_run->exit_reason, vcpu->kvm_run->io.port);

			switch (vcpu->kvm_run->exit_reason)
			{
			case 2:
				printf("KVM_EXIT_IO\n");
				break;

			case 5:
				printf("KVM_EXIT_HLT\n");
				break;

			default:
				printf("Add more cases in line %d\n", __LINE__);
				break;
			}
		}
		if (viva && VMSleep)
			sleep(10000);
		switch (vcpu->kvm_run->exit_reason)
		{

		case KVM_EXIT_HLT:
			// printf
			goto check;

		case KVM_EXIT_IO:
			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xE9)
			{
				char *p = (char *)vcpu->kvm_run;
				fwrite(p + vcpu->kvm_run->io.data_offset,
					   vcpu->kvm_run->io.size, 1, stdout);
				fflush(stdout);
				continue;
			}

			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xF1)
			{
				char *p = (char *)vcpu->kvm_run;
				printf("printVal() ==> %d\n\n", *(p + vcpu->kvm_run->io.data_offset));
				fflush(stdout);
				continue;
			}

			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_IN && vcpu->kvm_run->io.port == 0xF2)
			{
				printf("Exited for getNumExists()\n\n");
				fflush(stdout);
				*((char *)vcpu->kvm_run + vcpu->kvm_run->io.data_offset) = exit_count;
				continue;
			}

			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xF3)
			{
				uint32_t* virtualAddressInVM = (uint32_t*)(intptr_t)(*(uint32_t*)((uint8_t*)vcpu->kvm_run + vcpu->kvm_run->io.data_offset));
				uint8_t* virtualAddressInHost= (uint8_t*) vm->mem + (uint64_t) virtualAddressInVM;
				printf("Display() ==> %hhn\n\n", virtualAddressInHost);
				fflush(stdout);
				continue;
			}
	 
	//fopen function
			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xF4)
			{
				uint32_t* virtualAddressInVM = (uint32_t*)(intptr_t)(*(uint32_t*)((uint8_t*)vcpu->kvm_run + vcpu->kvm_run->io.data_offset));
				uint8_t* virtualAddressInHost= (uint8_t*) vm->mem + (uint64_t) virtualAddressInVM;
				// printf("F4 %s\n\n", virtualAddressInHost);

				char *filename = (char *)virtualAddressInHost;
				printf("filename: %s\n",filename);
				if (ioctl(vcpu->fd, KVM_RUN, 0) < 0)
				{
					perror("KVM_RUN");
					exit(1);
				}
				virtualAddressInVM = (uint32_t*)(intptr_t)(*(uint32_t*)((uint8_t*)vcpu->kvm_run + vcpu->kvm_run->io.data_offset));
				virtualAddressInHost= (uint8_t*) vm->mem + (uint64_t) virtualAddressInVM;
				// printf("F4 %s\n\n", virtualAddressInHost);


				char *mode = (char *)virtualAddressInHost;
				printf("mode: %s\n",mode);
				
				
				printf("curindex before: %d\n",curIndex);

				FILE *fd = fopen(filename,mode);
				// printf("fileno = %d\n",fileno(fd));
				FILEarray[curIndex]=fd;
				printf("entering: %d\n",curIndex);
				*((char *)vcpu->kvm_run + vcpu->kvm_run->io.data_offset) = curIndex;
				// *(uint32_t*)((uint8_t*)vcpu->kvm_run + vcpu->kvm_run->io.data_offset) =curIndex;
				vm->mem[0x400] = curIndex;

				curIndex++;
				printf("curindex after: %d\n",curIndex);

				if (ioctl(vcpu->fd, KVM_RUN, 0) < 0)
				{
					perror("KVM_RUN");
					exit(1);
				}

				fflush(stdout);
				continue;
			}


		if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xF5)
			{
				uint32_t* virtualAddressInVM = (uint32_t*)(intptr_t)(*(uint32_t*)((uint8_t*)vcpu->kvm_run + vcpu->kvm_run->io.data_offset));
				uint8_t* virtualAddressInHost= (uint8_t*) vm->mem + (uint64_t) virtualAddressInVM;
				// printf("F4 %s\n\n", virtualAddressInHost);

				char *content = (char *)virtualAddressInHost;
				char data[strlen(content)];
				unsigned int i;
				for(i=0;i<strlen(content);i++){
					data[i]=content[i];

				}
				data[i]='\0';
				printf("content: %s\n %s\n",content,data);

				if (ioctl(vcpu->fd, KVM_RUN, 0) < 0)
				{
					perror("KVM_RUN");
					exit(1);
				}

				char *p = (char *)vcpu->kvm_run;
				int index=*(p + vcpu->kvm_run->io.data_offset);


				FILE *fd=FILEarray[index];
				printf("Index ==> %d\n fileno = %d\n", index,fileno(fd));

				// fprintf(fd,"%s",content);
				fwrite(data,1,sizeof(data),fd);

				fflush(stdout);
				continue;

			}






//Read

		if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xF6)
			{

				char *p = (char *)vcpu->kvm_run;
				int size=*(p + vcpu->kvm_run->io.data_offset);

				printf("\nsize ==> %d\n", *(p + vcpu->kvm_run->io.data_offset));

				if (ioctl(vcpu->fd, KVM_RUN, 0) < 0)
				{
					perror("KVM_RUN");
					exit(1);
				}

				p = (char *)vcpu->kvm_run;
				int index=*(p + vcpu->kvm_run->io.data_offset);

				FILE *fd=FILEarray[index];
				printf("Index ==> %d\n fileno = %d\n", index,fileno(fd));

				char data[500]="";

				int sizeofdata = fread(data,1,size,fd);
				printf("data: %s\nsize of data: %d\n",data,sizeofdata);
				vm->mem[0x400]=&data;

				fflush(stdout);
				continue;

			}



 

			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.port == 0xF7)
			{
				char *p = (char *)vcpu->kvm_run;
				printf("printVal() ==> %d\n\n", *(p + vcpu->kvm_run->io.data_offset));
				int index=*(p + vcpu->kvm_run->io.data_offset);
				fclose(FILEarray[index]);
				fflush(stdout);
				continue;
			}









			/* fall through */
		default:
			fprintf(stderr, "Got exit_reason %d,"
							" expected KVM_EXIT_HLT (%d)\n",
					vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
			exit(1);
		
		}

	}

check:
	if (viva)
	{
		printf("42 is only to check proper termination of the guest OS, the guest OS writes 42 in its virtual memory ");
		printf("at addr 0x400 and also in its regiser rax hence can be modified to any other number\n");
	}
	if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0)
	{
		perror("KVM_GET_REGS");
		exit(1);
	}

	if (regs.rax != 42)
	{
		printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
		return 0;
	}

	memcpy(&memval, &vm->mem[0x400], sz);
	if (memval != 42)
	{
		printf("Wrong result: memory at 0x400 is %lld\n",
			   (unsigned long long)memval);
		return 0;
	}

	return 1;

}

extern const unsigned char guest16[], guest16_end[];

int run_real_mode(struct vm *vm, struct vcpu *vcpu)
{
	if (viva)

		printf("run_real_mode %d\n", __LINE__);

	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing real mode\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	sregs.cs.selector = 0;
	sregs.cs.base = 0;

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest16, guest16_end - guest16);
	return run_vm(vm, vcpu, 2);
}

static void setup_protected_mode(struct kvm_sregs *sregs)
{
	if (viva)

		printf("setup_protected_mode %d\n", __LINE__);

	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 1,
		.s = 1, /* Code/data */
		.l = 0,
		.g = 1, /* 4KB granularity */
	};

	sregs->cr0 |= CR0_PE; /* enter protected mode */

	sregs->cs = seg;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

extern const unsigned char guest32[], guest32_end[];

int run_protected_mode(struct vm *vm, struct vcpu *vcpu)
{
	if (viva)

		printf("run_protected_mode %d\n", __LINE__);

	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing protected mode\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest32, guest32_end - guest32);
	return run_vm(vm, vcpu, 4);
}

static void setup_paged_32bit_mode(struct vm *vm, struct kvm_sregs *sregs)
{
	if (viva)

		printf("setup_paged_32bit_mode %d\n", __LINE__);

	uint32_t pd_addr = 0x2000;
	uint32_t *pd = (void *)(vm->mem + pd_addr);

	/* A single 4MB page to cover the memory region */
	pd[0] = PDE32_PRESENT | PDE32_RW | PDE32_USER | PDE32_PS;
	/* Other PDEs are left zeroed, meaning not present. */

	sregs->cr3 = pd_addr;
	sregs->cr4 = CR4_PSE;
	sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs->efer = 0;
}

int run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu)
{
	if (viva)

		printf("run_paged_32bit_mode %d\n", __LINE__);

	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing 32-bit paging\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);
	setup_paged_32bit_mode(vm, &sregs);

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	memcpy(vm->mem, guest32, guest32_end - guest32);
	return run_vm(vm, vcpu, 4);
}

extern const unsigned char guest64[], guest64_end[];

static void setup_64bit_code_segment(struct kvm_sregs *sregs)
{
	if (viva)

		printf("setup_64bit_code_segment %d\n", __LINE__);

	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, /* Code: execute, read, accessed */
		.dpl = 0,
		.db = 0,
		.s = 1, /* Code/data */
		.l = 1,
		.g = 1, /* 4KB granularity */
	};

	if (viva)
		printf("Setting code section to %llu with limit %x line:%d\n",
			   seg.base, seg.limit, __LINE__);
	sregs->cs = seg;

	//testing
	// sregs->cs.base=7;
	// printf("test %lld",sregs->cs.);

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static void setup_long_mode(struct vm *vm, struct kvm_sregs *sregs)
{
	if (viva)

		printf("setup_long_mode %d\n", __LINE__);

	// printf("printing registers\n");
	// printf("%d",sregs->cs);

	uint64_t pml4_addr = 0x2000;
	if (viva)
		printf("Setting page map level 4 at %llx ie at %lx virtual address of guest OS\n", (long long int)(vm->mem + pml4_addr), pml4_addr);
	uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

	uint64_t pdpt_addr = 0x3000;
	if (viva)
		printf("Setting page directory pointer at %llx ie at %lx virtual address of guest OS\n", (long long int)(vm->mem + pdpt_addr), pdpt_addr);
	uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

	uint64_t pd_addr = 0x4000;
	if (viva)
		printf("Setting page table at %llx ie at %lx virtual address of guest OS\n", (long long int)(vm->mem + pd_addr), pd_addr);
	uint64_t *pd = (void *)(vm->mem + pd_addr);

	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;
	pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;

	sregs->cr3 = pml4_addr;
	sregs->cr4 = CR4_PAE;
	sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs->efer = EFER_LME | EFER_LMA;

	setup_64bit_code_segment(sregs);
}

int run_long_mode(struct vm *vm, struct vcpu *vcpu)
{
	if (viva)

		printf("run_long_mode %d\n", __LINE__);
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing 64-bit mode\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0)
	{
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_long_mode(vm, &sregs);

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0)
	{
		perror("KVM_SET_SREGS");
		exit(1);
	}

	if (viva)
		printf("Resetting VCPU registers in line number %d\n", __LINE__ + 1);
	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 0x2;
	regs.rip = 0;
	if (viva)
		printf("Guest starts executing from guest virtual address %llx configured in line %d\n",
			   regs.rip, __LINE__ - 3);
	/* Create stack at top of 2 MB page and grow down. */

	regs.rsp = 0x200000;

	if (viva)
	{
		printf("setting stack pointer to the end of the first page (ie at 2MB) at %x of guest memeory and at %lx in hypervisor in line %d\n", 2 << 20, (long int)(vm->mem + (2 << 20)), __LINE__ - 3);
		printf("This is the end of the physical memory of guest os and hence stack grows in the opposite direction\n");
	}
	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0)
	{
		perror("KVM_SET_REGS");
		exit(1);
	}

	if (viva)
		printf("copying guest code of size %ld to address %lx in hypervisor and 0 in guest os\n", guest64_end - guest64, (long int)vm->mem);
	memcpy(vm->mem, guest64, guest64_end - guest64);
	return run_vm(vm, vcpu, 8);
}

int main(int argc, char **argv)
{
	if (viva)

		printf("main %d\n", __LINE__);

	struct vm vm;
	struct vcpu vcpu;
	enum
	{
		REAL_MODE,
		PROTECTED_MODE,
		PAGED_32BIT_MODE,
		LONG_MODE,
	} mode = REAL_MODE;
	int opt;

	while ((opt = getopt(argc, argv, "rspl")) != -1)
	{
		switch (opt)
		{
		case 'r':
			mode = REAL_MODE;
			break;

		case 's':
			mode = PROTECTED_MODE;
			break;

		case 'p':
			mode = PAGED_32BIT_MODE;
			break;

		case 'l':
			mode = LONG_MODE;
			break;

		default:
			fprintf(stderr, "Usage: %s [ -r | -s | -p | -l ]\n",
					argv[0]);
			return 1;
		}
	}

	// printf("initiating vm with memory %d Mbytes\n",0x200000/1024/1024*getpagesize()/1024);

	vm_init(&vm, 0x40000000);
	vcpu_init(&vm, &vcpu);

	switch (mode)
	{
	case REAL_MODE:
		return !run_real_mode(&vm, &vcpu);

	case PROTECTED_MODE:
		return !run_protected_mode(&vm, &vcpu);

	case PAGED_32BIT_MODE:
		return !run_paged_32bit_mode(&vm, &vcpu);

	case LONG_MODE:
		return !run_long_mode(&vm, &vcpu);
	}

	return 1;
}
