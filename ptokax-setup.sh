#!/bin/bash

set -eou pipefail

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)
ORANGE=$(tput setaf 9)

echo -e "${GREEN}[+] ${BLUE}Enabling and Starting DHCP service${WHITE}"
sudo service dhcpcd start
sudo systemctl enable dhcpcd 2>/dev/null

# Adding the static ip config only if an entry doesn't already exist
STATIC_IP=$(grep -q '#Static IP for PtokaX' /etc/dhcpcd.conf && echo true || echo false)
if [ "$STATIC_IP" == "false" ]; then
	echo -e "${GREEN}[+] ${BLUE}Making RPi's IP address static${WHITE}"

	RASPI_IP=$(ip addr | grep inet | grep eth0 | cut -d' ' -f6)
	GATEWAY_IP=$(ip route | grep default | cut -d' ' -f3)
	DNS_ADDRESS=$(grep nameserver -m 1 /etc/resolv.conf | cut -d' ' -f2)

	cat <<- EOF >> /etc/dhcpcd.conf
		#Static IP for PtokaX
		interface eth0
		static ip_address=${RASPI_IP}
		static routers=${GATEWAY_IP}
		static domain_name_servers=${DNS_ADDRESS}
	EOF
else
	echo -e "${YELLOW}[-] ${BLUE}RPi's IP address is already static${WHITE}"
fi

for action in "start" "stop" "setup"; do
	if [ ! -f ~/ptokax-${action}.sh ]; then
		echo -e "${GREEN}[+] ${BLUE}Downloading PtokaX-${action} script${WHITE}"
		curl -s https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-${action}.sh -L -o ~/ptokax-${action}.sh
		if [ ! -x ~/ptokax-${action}.sh ]; then
			chmod +x ~/ptokax-${action}.sh
		fi
	else
		echo -e "${YELLOW}[-] ${BLUE}PokaX-${action}.sh script already exist${WHITE}"
	fi
done

echo -e "${GREEN}[+] ${BLUE}Installing Prerequisites${WHITE}"
# Curl for downloading source code
# mysql - required for scripts
# LUA 5.2.2 - Latest lua not installed as scripts are in lua version 5.2.2
sudo apt install -y curl liblua5.2-dev make g++ zlib1g-dev libtinyxml-dev default-libmysqlclient-dev lua-sql-mysql libcap2-bin

echo -e "${GREEN}[+] ${BLUE}Downloading PtokaX${WHITE}"
# Download PtokaX source code
if [ ! -f ~/ptokax-0.5.2.2-src.tgz ]; then
	curl -L -s https://github.com/sheharyaar/ptokax/releases/download/latest/ptokax-0.5.2.2-src.tgz -o ~/ptokax-0.5.2.2-src.tgz
fi
# Extract the archive
if [ ! -d ~/PtokaX ]; then  
	tar -xf ~/ptokax-0.5.2.2-src.tgz
fi
echo -e "${GREEN}[+] ${BLUE}Installing PtokaX${WHITE}"
# Make the program
cd PtokaX/ || (echo "cd to PtokaX failed" && exit)
make -f makefile-mysql lua52
sudo make install
cd ~ || (echo "cd to ~ failed" && exit)

echo -e "${GREEN}[+] ${BLUE}Setting up PtokaX${WHITE}"
~/PtokaX/PtokaX -m

if [ ! -d ~/ptokax-scripts ]; then
	echo -e "${GREEN}[+] ${BLUE}Downloading Hit Hi Fit Hai scripts${WHITE}"
	git clone https://github.com/sheharyaar/ptokax-scripts ~/ptokax-scripts
else
	echo -e "${YELLOW}[-] ${BLUE}Hit Hi Fit Hai scripts already exist${WHITE}"
fi
if [ ! -d ~/PtokaX/scripts ]; then
	cp ~/ptokax-scripts/* ~/PtokaX/scripts/ -rf
fi

echo -e "${GREEN}[+] ${BLUE}Enabling and starting PtokaX service${WHITE}"
sudo systemctl enable ptokax.service
sudo service ptokax.service start

echo -e "${GREEN}[+] ${BLUE}Enabling and starting PtokaX-DHCP service${WHITE}"
sudo systemctl enable ptokax-dhcp.service
sudo service ptokax-dhcp.service start

echo -e "${RED}[*] ${BLUE}Run PtokaX server using ${ORANGE}sudo ~/ptokax-start.sh${WHITE}"

echo -e "${GREEN}[+] ${BLUE}Rebooting Pi in 10 seconds ...${WHITE}"
sleep 10; sudo reboot
