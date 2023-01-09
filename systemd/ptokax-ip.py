import netifaces


def get_interface_adddress(itf_name):
    idx = -1

    for itf in netifaces.interfaces():
        if itf == itf_name:
            idx = netifaces.interfaces().index(itf)

    if idx == -1:
        raise Exception(f"No interface \"{itf_name}\" found")

    return netifaces.ifaddresses(itf_name)


if __name__ == "__main__":
    try:
        addrs = get_interface_adddress("eth0")
    except Exception as e:
        print("Error in get_interface_address :", e)
    else:
        ip = addrs.get(netifaces.AF_INET)
        if ip == "None":
            print("No ip address assigned")
        else:
            print("IP Address : ", ip[0]["addr"])
