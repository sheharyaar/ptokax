import netifaces
import time
from subprocess import check_output as execo
# import RPi.GPIO as PI


## Set the pin number for the LED screen
# LED_PIN = 7 #TODO: set the currect pin number
# # Set the mode of the pins
# PI.setmode(PI.BOARD)
# # Set the pin as output
# PI.setup(LED_PIN, PI.OUT)


class InterfaceNotFoundException(Exception):
    "Interface not found"
    pass


class IPNotFoundException(Exception):
    "IP Address not assigned to interface"
    pass


## Function to display a message on the LED screen
# def output_on_led(message):
#     # Turn on the LED screen
#     PI.output(LED_PIN, True)
#     # Output the message to the LED screen
#     print(message)
#     time.sleep(3)
#     # Turn off the LED screen
#     PI.output(LED_PIN, False)


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
        addr = get_interface_adddress("eth0")

    except InterfaceNotFoundException as e:
        print(e)
        exit(1)

    except IPNotFoundException as e:
        print(e)
        if retry > 0:
            # handling dhcp stuff
            '''
            execl("/usr/bin/sed", "sed", "-i", '/#Static IP for PtokaX/q;/#Static IP for PtokaX/d;', "/etc/dhcpcd.conf") -> replaces the main process image
            system("/usr/bin/sed -i '/#Static IP for PtokaX/q;/#Static IP for PtokaX/d;' /etc/dhcpcd.conf") -> deprecated
            '''
            ## removing static ip config lines and storing output
            execo(['/usr/bin/sed', '-i', '/#Static IP for PtokaX/,$d', '/etc/dhcpcd.conf'])
            output = f'Removed the incorrect STATIC IP configuration'
            print(output) # TODO: Remove this statement
            # output_on_led(output)
            
        print("Retrying in 15 seconds")
        time.sleep(15)
        main(retry + 1)
    else:
        print("IP Address : ", addr)
        ## start the setup ptokax script
        output = execo(['/home/pi/MetaHub/ptokax-setup.sh'])
        output = output.decode("utf-8").strip()
        print(output) # TODO: Remove this statement
        # output_on_led(output)

    # Clean up the GPIO
    # PI.cleanup()


if __name__ == "__main__":
    main(0)
