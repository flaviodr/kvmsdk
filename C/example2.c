#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>

virConnectPtr connect_to_hypervisor(){
	virConnectPtr conn;

	// Opens a read only connection to the hypervisor
	conn = virConnectOpenReadOnly("");
        if (conn == NULL) {
                fprintf(stderr, "Not able to connect to the localhost hypervisor\n");
                exit(1);
        }

	return conn;
}

int find_a_domain(virConnectPtr conn, char *domain){
	virDomainPtr dom;

	// Search for a virtual machine (aka domain) on the hypervisor
	// connection
	dom = virDomainLookupByName(conn, domain);
        if (!dom) {
                return 0;
        } else {
		return virDomainGetID(dom);
	}
}


virDomainInfo *get_timestamp(virConnectPtr conn, int numDomains, int *activeDomains){
	int i;

	virDomainInfo *domain_array;
        virDomainInfo info;
        virDomainPtr dom;
	virDomainMemoryStatPtr memstats = NULL; 

	// the array used to return information about domains
	domain_array = malloc(sizeof(virDomainInfo) * numDomains);

	// Structure used to grab memory informatoin
	memstats = malloc(VIR_DOMAIN_MEMORY_STAT_NR*sizeof(virDomainMemoryStatStruct));

	for (i = 0 ; i < numDomains ; i++) {
		dom = virDomainLookupByID(conn, activeDomains[i]);
		
		// Grab information about the domain memory statistics
		// VIR_DOMAIN_MEMORY_STAT_NR means that you want all the mem info
		virDomainMemoryStats(dom, memstats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		
		// Get generic domain information
		virDomainGetInfo(dom, &info);

		memcpy(&domain_array[i], &info, sizeof(virDomainInfo));

		// Replace the maxMem field with the information we want.
		// Keeping it in the domain info to move with the domainID during qsort()
		domain_array[i].maxMem = memstats[VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT].val/1024;
	}
	
	// Free the memstats structure but not domain_array yet (it will be returned)
	free(memstats);

	return domain_array;
}

// Function used by qsort() to compare time slice
int compare(const void *ptr1, const void *ptr2){
	const virDomainInfo *ptr1p = ptr1;
	const virDomainInfo *ptr2p = ptr2;

	if (ptr1p->cpuTime > ptr2p->cpuTime)
		return -1;
	else if (ptr1p->cpuTime < ptr2p->cpuTime)
		return 1;
	else 
		return 0;
}

// The application starts here
int main(int argc, char *argv[])
{
	virConnectPtr conn;
	int numDomains, i, sum = 0;
	int *activeDomains;
	virDomainPtr dom;
	virDomainInfoPtr info;
	virDomainInfo *array, *array2;

	info = malloc(sizeof(virDomainInfo));

	conn = connect_to_hypervisor();
	numDomains = virConnectNumOfDomains(conn);

	activeDomains = malloc(sizeof(int) * numDomains);
	virConnectListDomains(conn, activeDomains, numDomains);
	
	// Print header
	printf("                  Name    vCPU   Memory    Mem fault   Proprtional CPU Utilization\n");
	printf("----------------------------------------------------------------------------------\n");

	// Capture total CPU utilization and memory faults in two instants
	array = get_timestamp(conn, numDomains, activeDomains);
	sleep(1);
	array2 = get_timestamp(conn, numDomains, activeDomains);

	// Putting the CPU and Memory differences in the virDomainInfo for qsort()	
	for (i = 0 ; i < numDomains ; i++) {
		array[i].maxMem = array2[i].maxMem - array[i].maxMem;
		array[i].cpuTime = array2[i].cpuTime - array[i].cpuTime;
		sum += array[i].cpuTime;
	}

	// array2 is useless now on
	free(array2);
	
	// sort the machines by the most used one
	qsort(array, numDomains, sizeof(virDomainInfo), &compare);

	for (i = 0 ; i < numDomains ; i++) {
		dom = virDomainLookupByID(conn, activeDomains[i]);
		virDomainGetInfo(dom, info); 

		// Print domain information
		printf("%22s   %4d  ", virDomainGetName(dom), info->nrVirtCpu);

		// Print memory information
		printf("%7d      %4lu ", (int) info->memory/1024, array[i].maxMem/1024);

		// Print CPU utilization percentage
		printf("          %5.2f %% \n", (double) 100*array[i].cpuTime/sum);
	}

	// Let's free the malloced structures
	free(array);
	free(info);
	free(activeDomains);
	
	return 0;
}
