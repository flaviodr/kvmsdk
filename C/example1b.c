#include <stdio.h>
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

int main(int argc, char *argv[])
{
	virConnectPtr conn;
	int id;
	char *vm_name;

	if (argc < 2){
		printf("Usage\n%s <virtual machine name\n", argv[0]);
		exit(1);
	} else {
		vm_name = argv[1];
	}

	conn = connect_to_hypervisor();
	id = find_a_domain(conn, vm_name);
	virConnectClose(conn);

	if (id > 0){
		printf("Domain %s is running\n", vm_name);
	} else if (id < 0){
		printf("Domain %s is NOT running\n", vm_name);
	} else { 
		// If ID is 0, it means that the virtual machine does not exist
		// on the hypervisor
		printf("Domain %s not found on this hypervisor\n", vm_name);
	}

	return 0;
}
