#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// template for making your own GET requests
// -> makes "sample.txt" with the request. To run use netcat: nc localhost port_number < sample.txt

int main(void)
{
	
	FILE *fp;
	if((fp = fopen("sample.txt", "w")) == NULL){
		printf("ERROR\n");
		return -1;
	}
	fprintf(fp, "GET /site0/page0-4707.html HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nHost: www.tutorialspoint.com\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n\n");
	fclose(fp);
	return 0;
}
