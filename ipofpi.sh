#!/bin/bash

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
PURPLE=$(tput setaf 5)
WHITE=$(tput setaf 7)
ORANGE=$(tput setaf 9)

spinner() {
	local pid=$!
	local spin="|/-\\"
	local i=0
	while [ "$(ps a | awk '{print $1}' | grep $pid)" -eq "$pid" ]; do
		i=$(( (i+1)%4 ))
		printf " ${YELLOW}Scanning the network \r[${RED}%s${YELLOW}]${WHITE}" "${spin:$i:1}"
		sleep 0.25 
	done
	printf "\b\b\b"
}

internet_interface=$(ip addr | grep -Ev "127.0.0.1|scopeid|inet6" | grep "inet" | awk '{print $NF}')
if [ -z "$internet_interface" ]; then
	echo -e "${YELLOW}[-] ${BLUE}Internet interface: ${RED}<EMPTY>${WHITE}"
	echo -e "${RED}[ERROR]${WHITE}: You are NOT connected to the ${YELLOW}internet${WHITE}"
	exit
fi
echo -e "${GREEN}[+] ${BLUE}Internet interface: ${WHITE}$internet_interface"
subnet_address=$(ip route | grep -E "link|${internet_interface}" | cut -d ' ' -f 1 | grep "/24")
if [ -z "$subnet_address" ]; then
	echo -e "${YELLOW}[-] ${BLUE}Subnet address: ${RED}<EMPTY>${WHITE}"
	echo -e "${RED}[ERROR]${WHITE}: You are not connected to the ${ORANGE}ethernet${WHITE} (ONLY)"
	exit
fi
echo -e "${GREEN}[+] ${BLUE}Subnet address: ${WHITE}$subnet_address"

sudo nmap -v0 -sS -O -p22 "${subnet_address}" -oG /tmp/dcpp_pi_scan &
spinner
IPofPI=$(grep "open" < /tmp/dcpp_pi_scan | grep "OS: Linux " | awk '{print $2}')
if [ -z "$IPofPI" ]; then
	echo -e "${RED}[ERROR]${WHITE}: Failed to get the IP of pi."
	echo -e "${PURPLE}[DEBUG]${WHITE}: There cane be multiple reasons for a failure at this stage:
	   ${YELLOW}1.${WHITE} The pi is not connected to power outlet.
	   ${YELLOW}2.${WHITE} The pi is in shutdown state.
	   ${YELLOW}3.${WHITE} LAN cable is not connected to pi or the LAN port properly.
	   ${YELLOW}4.${WHITE} If you have changed Hall or even wing from last operation 
	      IP will not being assigned to it because of previous static ip configuration. 
	      To disable it - take out SD card; access the file system and then reverse the steps in: 
	      ${GREEN}https://www.codecnetworks.com/blog/implementing-static-ip-address-for-raspberry-pi/${WHITE}"
	exit
fi
sudo rm -f /tmp/dcpp_pi_scan
echo -e "${GREEN}[+] ${BLUE}IP of PI : ${WHITE}$IPofPI"
