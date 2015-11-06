#include <stdio.h>
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

int find_a_domain(virConnectPtr conn, char *domain)
{
    virDomainPtr dom;

    // search for a virtual machine (aka domain) on the hypervisor
    // connection
    dom = virDomainLookupByName(conn, domain);
    if (!dom)
        return 0;

    return virDomainGetID(dom);
}

int main(int argc, char *argv[])
{
    virConnectPtr conn;
    int id;
    char *vm_name;

    if (argc < 2) {
        printf("Usage: %s <virtual machine name>\n", argv[0]);
        exit(1);
    }

    vm_name = argv[1];

    conn = connect_to_hypervisor();
    id = find_a_domain(conn, vm_name);
    virConnectClose(conn);

    // guest is running
    if (id > 0)
        printf("Domain %s is running\n", vm_name);

    // guest is not running
    else if (id < 0)
        printf("Domain %s is NOT running\n", vm_name);

    // guest does not exist
    else
        printf("Domain %s not found on this hypervisor\n", vm_name);

    return 0;
}
