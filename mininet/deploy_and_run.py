from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from topology import custom_topology
import time 

def deploy_and_run(net):
    print("Deploying binaries to Mininet hosts...")

    # cd into the client directory
    net.get('h1').cmd('cd client/')

    # cd into the router directory
    net.get('h2').cmd('cd router/')
 
    # Run make clean and make on the server directorie
    net.get('h3').cmd('cd server/')
    net.get('h3').cmd('make clean')
    net.get('h3').cmd('make')
    net.get('h3').cmd('cd ..')

    # Start servers on h3, h4, h5
    for host in ['h3', 'h4', 'h5']:
        net.get(host).cmd('cd server/')
        net.get(host).cmd('./server &')

    print("Starting programs on Mininet hosts...")

    # Start router on h2
    net.get('h2').cmd('make clean')
    net.get('h2').cmd('make')
    net.get('h2').cmd('./router &')        

    print("Starting client operations...")

    # remake the client binaries
    net.get('h1').cmd('make clean')
    net.get('h1').cmd('make')
    time.sleep(1)
    
    # Run client operations
    print("Running --store operation...")
    client_output_store = net.get('h1').cmd('./client --store 10.0.0.2')
    print(client_output_store)

    time.sleep(1)
    print("Running --retrieve operation...")
    client_output_retrieve = net.get('h1').cmd('./client --retrieve 10.0.0.2')
    print(client_output_retrieve) 

if __name__ == '__main__':
    net = custom_topology()
    deploy_and_run(net)
    CLI(net)
    net.stop()
