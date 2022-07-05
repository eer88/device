#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char *argv[])
{
	int fs;
	int ret;
	unsigned char buf[1];
	if(3!=argc)
	{
		printf("Usage:\n"
				"\t./ledApp /dev/led 1 @ close LED\n"
				"\t./ledApp /dev/led 0 @ open LED\n");
		return -1;
	}
	/*打开设备*/
	fs=open(argv[1],O_RDWD);
	if(0>fs)
	{
		printf("open %s failed!",argv[1]);
		return -1;
	}
	
	buf[0]=atoi(argv[2]);
	ret=write(fs,buf,sizeof(buf));
	if(0>ret)
	{
		printf("contrl led failed!\n");
		close(fs);
		return -1;
	}
	close(fs);
	return 0;
	
}