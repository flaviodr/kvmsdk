#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>

#define SCREEN_COLS 76

virConnectPtr connect_to_hypervisor()
{
	virConnectPtr conn;

	// Opens a read only connection to the hypervisor
	conn = virConnectOpenReadOnly("");
    if (conn == NULL) {
        fprintf(stderr, "Not able to connect to the localhost hypervisor\n");
        exit(1);
    }

	return conn;
}

int find_a_domain(virConnectPtr conn, char *domain)
{
	virDomainPtr dom;

	// Search for a virtual machine (aka domain) on the hypervisor
	// connection
	dom = virDomainLookupByName(conn, domain);
    if (!dom)
        return 0;

    return virDomainGetID(dom);
}


virDomainInfo *get_guest_info(virConnectPtr conn,
                              int numDomains,
                              int *activeDomains)
{
	int i;

	virDomainInfo *domain_array;
    virDomainInfo info;
    virDomainPtr dom;
    virDomainMemoryStatStruct memstats[VIR_DOMAIN_MEMORY_STAT_NR];

	// the array used to return information about domains
	domain_array = (virDomainInfo*)malloc(sizeof(virDomainInfo) * numDomains);
    if (domain_array == NULL) {
        fprintf(stderr, "Cannot allocate memory to domain_array.\n");
        virConnectClose(conn);
        exit(1);
    }

	for (i = 0 ; i < numDomains ; i++) {
		dom = virDomainLookupByID(conn, activeDomains[i]);

		// Grab information about the domain memory statistics
		// VIR_DOMAIN_MEMORY_STAT_NR means that you want all the mem info
		virDomainMemoryStats(dom,
                (virDomainMemoryStatPtr)&memstats,
                VIR_DOMAIN_MEMORY_STAT_NR,
                0);

		// Get generic domain information
		virDomainGetInfo(dom, &info);

		memcpy(&domain_array[i], &info, sizeof(virDomainInfo));

		// Replace the maxMem field with the information we want.
		// Keeping it in the domain info to move with the domainID
        // during qsort()
		domain_array[i].maxMem =
            memstats[VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT].val / 1024;
	}

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

	return 0;
}

// The application starts here
int main(int argc, char *argv[])
{
	virConnectPtr conn;
	int numDomains, i;
	int *activeDomains;
    long long sum = 0;
	virDomainPtr dom;
	virDomainInfo info;
	virDomainInfo *fst_measure, *snd_measure;

	conn = connect_to_hypervisor();

	// Get the Number of guests on that hypervisor
	numDomains = virConnectNumOfDomains(conn);

	// Get all active domains
	activeDomains = malloc(sizeof(int) * numDomains);
	virConnectListDomains(conn, activeDomains, numDomains);

	// Print header
	printf("%-15s | %4s | %7s | %9s | %28s\n",
           "Name", "vCPU", "Memory", "Mem fault",
           "Proportional CPU Utilization");
    for (i = 0; i < SCREEN_COLS; ++i)
        printf("-");
    printf("\n");

	// Capture total CPU utilization and memory faults in two instants
	fst_measure = get_guest_info(conn, numDomains, activeDomains);

	// Sleep for 1 second in order to grab the VM stats during this time
	sleep(1);
	snd_measure = get_guest_info(conn, numDomains, activeDomains);

	// Putting the CPU and Memory differences in the virDomainInfo for qsort()
	for (i = 0 ; i < numDomains ; i++) {
		fst_measure[i].maxMem = snd_measure[i].maxMem - fst_measure[i].maxMem;
		fst_measure[i].cpuTime = snd_measure[i].cpuTime - fst_measure[i].cpuTime;
		sum += fst_measure[i].cpuTime;
	}

	// second measurement is useless from now on
	free(snd_measure);

	// sort the machines by the most used one
	qsort(fst_measure, numDomains, sizeof(virDomainInfo), &compare);

	// List all the domains and print the info for each
	for (i = 0 ; i < numDomains ; i++) {
		dom = virDomainLookupByID(conn, activeDomains[i]);
		virDomainGetInfo(dom, &info);

		// Print domain information
		printf("%*.*s | %4d | ", 15, 15, virDomainGetName(dom), info.nrVirtCpu);

		// Print memory information
		printf("%7d | %9lu | ", (int)info.memory / 1024,
                             fst_measure[i].maxMem / 1024);

		// Print CPU utilization percentage
		printf("%13.2f %%\n", (double)100 * fst_measure[i].cpuTime / sum);
	}

	// Let's free the malloced structures
	free(fst_measure);
	free(activeDomains);

    virConnectClose(conn);

	return 0;
}
