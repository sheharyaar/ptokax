#!/bin/bash

printf "\n\n[log] Enabling and Starting DHCP Service\n\n"

sudo service dhcpcd start
sudo systemctl enable dhcpcd

RASPI_IP=$(ip addr | grep inet | grep eth0 | cut -d ' ' -f 6)
printf "RASPI IP : ${RASPI_IP}\n"

GATEWAY_IP=$(ip route | grep default | cut -d ' ' -f 3)
printf "GATEWAY_IP : ${GATEWAY_IP}\n"

DNS_ADDRESS=$(cat /etc/resolv.conf | grep nameserver -m 1 | cut -d ' ' -f 2)
printf "DNS DNS_ADDRESS : ${DNS_ADDRESS}\n"

printf "\n\nMaking Raspberry Pi IP Address static\n"

sudo echo "#Static IP for PtokaX" >> /etc/dhcpcd.conf
sudo echo "interface eth0" >> /etc/dhcpcd.conf
sudo echo "static ip_address=${RASPI_IP}" >> /etc/dhcpcd.conf
sudo echo "static routers=${GATEWAY_IP}" >> /etc/dhcpcd.conf
sudo echo "static domain_name_servers=${DNS_ADDRESS}" >> /etc/dhcpcd.conf

home=$(echo $HOME)

cd $home

printf "\n\n[log] Downloading start and stop scripts\n\n"

curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-start.sh -L -o ./ptokax-start.sh
chmod +x ./ptokax-start.sh

curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-stop.sh -L -o ./ptokax-stop.sh
chmod +x ./ptokax-stop.sh

curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-config.sh -L -o ./ptokax-config.sh
chmod +x ./ptokax-config.sh

printf "\n\n[log] Installing Prerequisites\n\n"

# Install curl to download source code
sudo apt install -y curl

# Install g++, zlib, tinyxml
sudo apt install -y make g++ zlib1g-dev libtinyxml-dev

# Install lua 5.2.2
# Latest lua not installed as scripts are in lua version 5.2.2
sudo apt install -y liblua5.2-dev

# Install mysql - needed for scripts
sudo apt-get install -y default-libmysqlclient-dev lua-sql-mysql

# Go back to home
cd $home 

printf "\n\n[log] Downloading and installing PtokaX\n\n"

# Download PtokaX source code
curl -L https://github.com/sheharyaar/ptokax/releases/download/latest/ptokax-0.5.2.2-src.tgz -o ./ptokax-0.5.2.2-src.tgz

# Extract the archive
tar -xf ./ptokax-0.5.2.2-src.tgz

# Make the program
cd PtokaX/
make -f makefile-mysql lua52

# Setup privileges in order to allow ptokax to run on port 411 (default port)
sudo apt install -y libcap2-bin
sudo make install

printf "\n\n[log] Setting up PtokaX!\n\n"

./PtokaX -m

printf "\n\n[log] Now importing Hit Hi Fit Hai scripts\n\n"

cd $home
git clone https://github.com/sheharyaar/ptokax-scripts
cp ptokax-scripts/* PtokaX/scripts/ -rf

printf "\n\nRun Ptokax server using \"sudo ~/ptokax-start.sh\" \n"

printf "\n\nRestarting Raspberry Pi in 10 seconds\n"

sleep 10

sudo reboot
