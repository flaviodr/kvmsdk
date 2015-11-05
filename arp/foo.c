#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *ip_for_mac(char *mac){
	char line[1024];
	char ipAddr[24], hwAddr[48];
	char *ret;

	FILE *arp_table = fopen("/proc/net/arp", "r");
	if (arp_table == NULL){
		perror("Not able to open /proc/net/arp\n");
	}

	ret = malloc(1024*sizeof(char));

	//ignore reader
	fgets(line, sizeof(line), arp_table);
	
	while (fgets(line, 1024, arp_table)){
		if (strstr(line, mac))
			strcpy(ret, strtok(line, " "));
	}

	close(arp_table);

	return ret;
}
	
int main()
{
	char *ip;

	ip = ip_for_mac("52:54:00:28:3f:7f");
	printf("%s\n", ip);

	free(ip);

	return 0;
}
