#!/bin/bash

set -eou pipefail

printf "\n\n[log] Enabling and Starting DHCP Service\n\n"
sudo service dhcpcd start
sudo systemctl enable dhcpcd

# Adding the static ip config only if an entry doesn't already exist
grep -q '#Static IP for PtokaX' sed_ptokax.conf
if [ "$?" -eq 1 ]; then
	printf "\n\nMaking Raspberry Pi IP Address static\n"

	RASPI_IP=$(ip addr | grep inet | grep eth0 | cut -d ' ' -f 6)
	GATEWAY_IP=$(ip route | grep default | cut -d ' ' -f 3)
	DNS_ADDRESS=$(grep nameserver -m 1 /etc/resolv.conf | cut -d' ' -f2)

	cat <<- EOF >> /etc/dhcpcd.conf
		#Static IP for PtokaX
		interface eth0
		static ip_address=${RASPI_IP}
		static routers=${GATEWAY_IP}
		static domain_name_servers=${DNS_ADDRESS}
	EOF
fi

printf "\n\n[log] Downloading start and stop scripts\n\n"
for action in "start" "stop" "setup"; do
	curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-${action}.sh -L -o ~/ptokax-${action}.sh
	chmod +x ./ptokax-${action}.sh
done

printf "\n\n[log] Installing Prerequisites\n\n"
# Curl for downloading source code
# mysql - required for scripts
# LUA 5.2.2 - Latest lua not installed as scripts are in lua version 5.2.2
sudo apt install -y curl liblua5.2-dev make g++ zlib1g-dev libtinyxml-dev default-libmysqlclient-dev lua-sql-mysql

printf "\n\n[log] Downloading and installing PtokaX\n\n"
# Download PtokaX source code
curl -L https://github.com/sheharyaar/ptokax/releases/download/latest/ptokax-0.5.2.2-src.tgz -o ~/ptokax-0.5.2.2-src.tgz
# Extract the archive
tar -xf ~/ptokax-0.5.2.2-src.tgz
# Make the program
cd PtokaX/ || (echo "cd to PtokaX failed" && exit)
make -f makefile-mysql lua52
# Setup privileges in order to allow ptokax to run on port 411 (default port)
sudo apt install -y libcap2-bin
sudo make install

printf "\n\n[log] Setting up PtokaX!\n\n"
./PtokaX -m

printf "\n\n[log] Now importing Hit Hi Fit Hai scripts\n\n"
git clone https://github.com/sheharyaar/ptokax-scripts ~/ptokax-scripts
cp ~/ptokax-scripts/* ~/PtokaX/scripts/ -rf

printf "\n\nRun Ptokax server using \"sudo ~/ptokax-start.sh\" \n"
sudo systemctl enable ptokax-dhcp.service
sudo service ptokax-dhcp.service start

printf "\n\nRestarting Raspberry Pi in 10 seconds\n"
sleep 10; sudo reboot
