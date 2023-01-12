import netifaces
import time
from subprocess import run


class InterfaceNotFoundException(Exception):
    "Interface not found"
    pass


class IPNotFoundException(Exception):
    "IP Address not assigned to interface"
    pass


def get_interface_adddress(itf_name):
    idx = -1

    for itf in netifaces.interfaces():
        if itf == itf_name:
            idx = netifaces.interfaces().index(itf)

    # Check if interface exists
    if idx == -1:
        raise InterfaceNotFoundException(f"Interface \"{itf_name}\" not found")

    addrs = netifaces.ifaddresses(itf_name)

    # Check if ip address exists
    if netifaces.AF_INET not in addrs.keys():
        raise IPNotFoundException("IP Address not assigned to interface")

    ip = addrs.get(netifaces.AF_INET)
    return ip[0]["addr"]


def main(retry):
    try:
        addr = get_interface_adddress("eno2")

    except InterfaceNotFoundException as e:
        print(e)
        exit(1)

    except IPNotFoundException as e:
        print(e)
        if retry > 0:
            # handle dhcp stuff
            ## removing static ip config lines
            # execl("/usr/bin/sed", "sed", "-i", '/#Static IP for PtokaX/q;/#Static IP for PtokaX/d;', "/etc/dhcpcd.conf") -> replaces the main process image
            # system("/usr/bin/sed -i '/#Static IP for PtokaX/q;/#Static IP for PtokaX/d;' /etc/dhcpcd.conf") -> deprecated
            run(["/usr/bin/sed", "-i", '/#Static IP for PtokaX/q;/#Static IP for PtokaX/d;', "/etc/dhcpcd.conf"])
            ## set new static ip
            ## start the ptokax service
            exit(1)

        print("Retrying in 15 seconds")
        time.sleep(15)
        main(retry + 1)
    else:
        print("IP Address : ", addr)


if __name__ == "__main__":
    main(0)
