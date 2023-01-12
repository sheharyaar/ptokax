#!/bin/bash

set -eou pipefail

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)
ORANGE=$(tput setaf 9)

spinner() {
	local pid=$!
	local spin="|/-\\"
	local i=0
	while [ "$(ps a | awk '{print $1}' | grep $pid)" -ne "$pid" ]; do
		i=$(( (i+1)%4 ))
		printf " ${YELLOW}$1 \r[${RED}%s${YELLOW}]${WHITE}" "${spin:$i:1}"
		sleep 0.25 
	done
	printf "\b\b\b"
}

echo -e "${GREEN}[+] ${BLUE}Enabling and Starting DHCP service${WHITE}"
sudo service dhcpcd start
sudo systemctl enable dhcpcd

# Adding the static ip config only if an entry doesn't already exist
grep -q '#Static IP for PtokaX' sed_ptokax.conf
if [ "$?" -eq 1 ]; then
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

echo -e "${GREEN}[+] ${BLUE}Downloading PtokaX - start, stop & setup scripts${WHITE}"
for action in "start" "stop" "setup"; do
	curl -s https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-${action}.sh -L -o ~/ptokax-${action}.sh
	chmod +x ~/ptokax-${action}.sh
done

echo -e "${GREEN}[+] ${BLUE}Installing Prerequisites${WHITE}"
# Curl for downloading source code
# mysql - required for scripts
# LUA 5.2.2 - Latest lua not installed as scripts are in lua version 5.2.2
sudo apt install -y curl liblua5.2-dev make g++ zlib1g-dev libtinyxml-dev default-libmysqlclient-dev lua-sql-mysql

echo -e "${GREEN}[+] ${BLUE}Downloading PtokaX${WHITE}"
# Download PtokaX source code
curl -L -s https://github.com/sheharyaar/ptokax/releases/download/latest/ptokax-0.5.2.2-src.tgz -o ~/ptokax-0.5.2.2-src.tgz
# Extract the archive
tar -xf ~/ptokax-0.5.2.2-src.tgz
echo -e "${GREEN}[+] ${BLUE}Installing PtokaX${WHITE}"
# Make the program
cd PtokaX/ || (echo "cd to PtokaX failed" && exit)
make -f makefile-mysql lua52
# Setup privileges in order to allow ptokax to run on port 411 (default port)
sudo apt install -y libcap2-bin
sudo make install

echo -e "${GREEN}[+] ${BLUE}Setting up PtokaX${WHITE}"
./PtokaX -m

echo -e "${GREEN}[+] ${BLUE}Downloading Hit Hi Fit Hai scripts${WHITE}"
git clone https://github.com/sheharyaar/ptokax-scripts ~/ptokax-scripts
cp ~/ptokax-scripts/* ~/PtokaX/scripts/ -rf

echo -e "${GREEN}[+] ${BLUE}Enabling and starting PtokaX service${WHITE}"
sudo systemctl enable ptokax-dhcp.service
sudo service ptokax-dhcp.service start

echo -e "${RED}[*] ${BLUE}Run PtokaX server using ${ORANGE}sudo ~/ptokax-start.sh${WHITE}"

echo -e "${GREEN}[+] ${BLUE}Rebooting Pi in 10 seconds ...${WHITE}"
sleep 10; sudo reboot
