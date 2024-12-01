from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink

def custom_topology():
    net = Mininet(controller=Controller, link=TCLink)

    # Add hosts
    client = net.addHost('h1', ip='10.0.0.1/24')  # Client in subnet 10.0.0.0/24
    router = net.addHost('h2')  # Router without default IP
    server1 = net.addHost('h3', ip='10.0.1.1/24') # Server 1 in subnet 10.0.1.0/24
    server2 = net.addHost('h4', ip='10.0.2.1/24') # Server 2 in subnet 10.0.2.0/24
    server3 = net.addHost('h5', ip='10.0.3.1/24') # Server 3 in subnet 10.0.3.0/24

    # Add links
    net.addLink(client, router)
    net.addLink(router, server1)
    net.addLink(router, server2)
    net.addLink(router, server3)

    # Start network
    net.start()

    # Assign IPs to router interfaces
    router.cmd('ifconfig h2-eth0 10.0.0.2/24')  # Interface connected to client
    router.cmd('ifconfig h2-eth1 10.0.1.2/24')  # Interface connected to server1
    router.cmd('ifconfig h2-eth2 10.0.2.2/24')  # Interface connected to server2
    router.cmd('ifconfig h2-eth3 10.0.3.2/24')  # Interface connected to server3

    # Enable IP forwarding on the router
    router.cmd('sysctl -w net.ipv4.ip_forward=1')

    # Configure default gateways for servers
    server1.cmd('route add default gw 10.0.1.2')
    server2.cmd('route add default gw 10.0.2.2')
    server3.cmd('route add default gw 10.0.3.2')

    # Configure default gateway for the client
    client.cmd('route add default gw 10.0.0.2')

    return net  # Ensure the Mininet object is returned

if __name__ == '__main__':
    net = custom_topology()
    CLI(net)
    net.stop()