#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libvirt/libvirt.h>

#define MAC_ADDRESS_LEN 17
#define IP_ADDRESS_LEN 15

typedef struct mac_addresses
{
    char addr[MAC_ADDRESS_LEN + 1];
    struct mac_addresses *next;
} mac_t;

virConnectPtr connect_to_hypervisor()
{
    // opens a read only connection to libvirt
    virConnectPtr conn = virConnectOpenReadOnly("");
    if (conn == NULL) {
        fprintf(stderr, "Not able to connect to libvirt.\n");
        exit(1);
    }
    return conn;
}

void get_domain_xml(const virConnectPtr conn,
                    const char *domain,
                    char **domain_xml)
{
    if (conn == NULL || domain == NULL) {
        fprintf(stderr, "get_domain_xml parameters are null.\n");
        exit(1);
    }

    // search for a virtual machine (aka domain) on the hypervisor
    // connection
    virDomainPtr dom = virDomainLookupByName(conn, domain);
    if (dom == NULL) {
        fprintf(stderr, "Guest %s cannot be found\n", domain);
        exit(1);
    }

    *domain_xml = virDomainGetXMLDesc(dom, VIR_DOMAIN_XML_UPDATE_CPU);

    virDomainFree(dom);
}

mac_t *get_macs(const char *domain_xml)
{
    if (domain_xml == NULL) {
        fprintf(stderr, "domain_xml parameter cannot be null.\n");
        exit(1);
    }

    mac_t *macs = NULL;
    char tmp_mac[MAC_ADDRESS_LEN + 1];

    // read the XML line by line looking for the MAC Address definition
    // and insert that address, if found, into a list of mac address
    // to be returned
    char *xml = strdup(domain_xml);
    char *token = strtok(xml, "\n");
    while (token) {

        if (sscanf(token, "%*[ ]<mac address='%17s", tmp_mac)) {
            mac_t *mac = (mac_t*)malloc(sizeof(mac_t));
            strncpy(mac->addr, tmp_mac, MAC_ADDRESS_LEN);
            mac->addr[MAC_ADDRESS_LEN] = '\0';
            mac->next = macs;
            macs = mac;
        }
        token = strtok(NULL, "\n");
    }
    free(xml);

    return macs;
}

void print_ip_per_mac(mac_t *head)
{
    if (head == NULL)
        return;

    char line[1024];
    mac_t *tmp = head;

    // open the arp file for read, this is where the IP addresses
    // will be found
    FILE *arp_table = fopen("/proc/net/arp", "r");
    if (arp_table == NULL) {
        fprintf(stderr, "Not able to open /proc/net/arp\n");
        exit(1);
    }

    // for each MAC address in the list, try to find the respective
    // IP address in the arp file, so print the MAC address and its
    // IP address to output
    printf("%-17s\t%s\n", "MAC Address", "IP Address");
    while (tmp != NULL) {
        while (fgets(line, 1024, arp_table)) {

            if (!strstr(line, tmp->addr))
                continue;

            printf("%s:\t%s\n", tmp->addr, strtok(line, " "));
            break;
        }
        fseek(arp_table, 0, SEEK_SET);
        tmp = tmp->next;
    }

    fclose(arp_table);
}

int main(int argc, char *argv[])
{
    // checks if all parameters are set
	if (argc != 2){
		printf("Usage: %s <guest name>\n", argv[0]);
		exit(1);
	}

    char *domain_xml    = NULL;
    mac_t *macs         = NULL;
	const char *vm_name = argv[1];
	virConnectPtr conn  = connect_to_hypervisor();

    // locates the guest and gets the XML which defines such guest
	get_domain_xml(conn, vm_name, &domain_xml);

    // gets a list with all mac address found in that particular guest
    macs = get_macs(domain_xml);

    // prints all IP address found for that guest
    print_ip_per_mac(macs);

    // clean all resources
    while (macs != NULL) {
        mac_t *tmp = macs->next;
        free(macs);
        macs = tmp;
    }
	free(domain_xml);
	virConnectClose(conn);

    return 0;
}
