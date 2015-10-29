import libvirt
import sys

def connect_to_hypervisor():
	# Opens a read only connection to the hypervisor
	connection = libvirt.openReadOnly(None)
	if connection is None:
		print 'Not able to connect to the localhost hypervisor'
		sys.exit(1)

	return connection

def find_a_domain(conn, domain):
	try:
		# Search for a virtual machine (aka domain) on the hypervisor
		# connection
		dom = conn.lookupByName(domain)
	except:
		# Virtual machine not found, returning 0
		return 0

	return dom.ID()

## Your code starts here
if __name__ == "__main__":
	if len(sys.argv) < 2:
		print "Usage\n%s <virtual machine name>" % sys.argv[0]
		sys.exit(1)

	vm_name = sys.argv[1]
	con = connect_to_hypervisor()
	ID = find_a_domain(con, vm_name)

	# close the open connection
	con.close()

	if ID > 0: 
		print "Domain %s is running " % vm_name
	elif ID < 0:
		print "Domain %s is NOT running " % vm_name
	else: 
		# If ID is 0, it means that the virtual machine does not exist
		# on the hypervisor
		print "Domain %s not found on this hypervisor " % vm_name

