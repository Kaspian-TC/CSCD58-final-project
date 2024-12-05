from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from topology import custom_topology

if __name__ == '__main__':
    net = custom_topology()
    CLI(net)
    net.stop()
