#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>

virConnectPtr connect_to_hypervisor()
{
    virConnectPtr conn;

    // opens a read only connection to the hypervisor
    conn = virConnectOpenReadOnly("");
    if (conn == NULL) {
        fprintf(stderr, "Not able to connect to the localhost hypervisor\n");
        exit(1);
    }

    return conn;
}

char *find_a_domain(virConnectPtr conn, char *domain){
	virDomainPtr dom;
	char *xml, *mac;

	mac = malloc(sizeof(char)*64);

	// Search for a virtual machine (aka domain) on the hypervisor
	// connection
	dom = virDomainLookupByName(conn, domain);
        if (!dom) {
                return 0;
        } 

	xml = virDomainGetXMLDesc(dom, VIR_DOMAIN_XML_UPDATE_CPU);

	xml = strstr(xml, "<mac address='");
	sscanf(xml, "<mac address='%17s", mac);

	return mac;
}

char *ip_for_mac(char *mac){
	char line[1024];
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

	fclose(arp_table);

	return ret;
}

int main(int argc, char *argv[])
{
	virConnectPtr conn;
	char *ip;
	char *mac;
	char *vm_name;

	if (argc < 2){
		printf("Usage\n%s <virtual machine name>\n", argv[0]);
		exit(1);
	} else {
		vm_name = argv[1];
	}

	conn = connect_to_hypervisor();
	mac = find_a_domain(conn, vm_name);
	ip = ip_for_mac(mac);

	printf("Current IP address for guest %s is %s\n", vm_name, ip);

	free(mac);
	virConnectClose(conn);
	return 0;
}
