import netifaces


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


if __name__ == "__main__":
    try:
        addr = get_interface_adddress("eth0")
    except Exception as e:
        print("Error in get_interface_address :", e)
    else:
        print("IP Address : ", addr)
