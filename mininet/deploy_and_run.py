from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from topology import custom_topology  # Ensure this imports your `custom_topology` function

def deploy_and_run(net):
    print("Deploying binaries to Mininet hosts...")

    # Deploy client binary to h1
    print("Deploying client to h1...")
    net.get('h1').cmd('mkdir -p /tmp/project')
    net.get('h1').cmd('cp /home/mininet/mininet_project/client/client /tmp/project/')

    # Deploy router binary to h2
    print("Deploying router to h2...")
    net.get('h2').cmd('mkdir -p /tmp/project')
    net.get('h2').cmd('cp /home/mininet/mininet_project/router/router /tmp/project/')

    # Deploy server binary to h3, h4, h5
    print("Deploying server to h3, h4, h5...")
    for host in ['h3', 'h4', 'h5']:
        net.get(host).cmd('mkdir -p /tmp/project')
        net.get(host).cmd(f'cp /home/mininet/mininet_project/server/server /tmp/project/')

    print("Starting programs on Mininet hosts...")

    # Start router on h2
    print("Starting router on h2...")
    net.get('h2').cmd('/tmp/project/router &')

    # Start server on h3, h4, h5
    for host in ['h3', 'h4', 'h5']:
        print(f"Starting server on {host}...")
        net.get(host).cmd('/tmp/project/server &')

    print("Starting client operations...")

    # Run client --store and --retrieve operations on h1
    client = net.get('h1')
    print("Running --store operation...")
    client_output_store = client.cmd('/tmp/project/client --store 10.0.0.2')
    print(client_output_store)

    print("Running --retrieve operation...")
    client_output_retrieve = client.cmd('/tmp/project/client --retrieve 10.0.0.2')
    print(client_output_retrieve)

if __name__ == '__main__':
    # Create the network using the topology defined in topology.py
    net = custom_topology()

    # Deploy binaries and run programs
    deploy_and_run(net)

    # Open CLI for manual debugging if needed
    CLI(net)

    # Stop the network after use
    net.stop()