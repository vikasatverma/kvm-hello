#include<stdio.h>
static void display(const char *str){
    
	printf("%x\n",str);
}

int main(int argc, char const *argv[])
{
    
	const char *str = "This is a random string";
	display(str);
    return 0;
}
