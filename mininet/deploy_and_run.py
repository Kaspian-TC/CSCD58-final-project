from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from topology import custom_topology

def deploy_and_run(net):
    print("Deploying binaries to Mininet hosts...")

    # Deploy client binary to h1
    net.get('h1').cmd('mkdir -p /tmp/project')
    net.get('h1').cmd('cp /home/mininet/mininet_project/client/client /tmp/project/')
    net.get('h1').cmd('chmod +x /tmp/project/client')

    # Deploy router binary to h2
    net.get('h2').cmd('mkdir -p /tmp/project')
    net.get('h2').cmd('cp /home/mininet/mininet_project/router/router /tmp/project/')
    net.get('h2').cmd('chmod +x /tmp/project/router')

    # Deploy server binaries to h3, h4, h5
    for host in ['h3', 'h4', 'h5']:
        net.get(host).cmd('mkdir -p /tmp/project')
        net.get(host).cmd(f'cp /home/mininet/mininet_project/server/server /tmp/project/')
        net.get(host).cmd('chmod +x /tmp/project/server')

    print("Starting programs on Mininet hosts...")

    # Start router on h2
    net.get('h2').cmd('/tmp/project/router &')

    # Start servers on h3, h4, h5
    for host in ['h3', 'h4', 'h5']:
        net.get(host).cmd('/tmp/project/server &')

    print("Starting client operations...")

    # Run client operations
    print("Running --store operation...")
    client_output_store = net.get('h1').cmd('/tmp/project/client --store 10.0.0.2')
    print(client_output_store)

    print("Running --retrieve operation...")
    client_output_retrieve = net.get('h1').cmd('/tmp/project/client --retrieve 10.0.0.2')
    print(client_output_retrieve)

if __name__ == '__main__':
    net = custom_topology()
    deploy_and_run(net)
    CLI(net)
    net.stop()
