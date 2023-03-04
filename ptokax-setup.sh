#!/bin/bash

set -eou pipefail

# Colors - Obvious no?
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)
ORANGE=$(tput setaf 9)

# IP of Pi
RASPI_IP=$(ip addr | grep inet | grep eth0 | cut -d' ' -f6)

if [ "$(systemctl is-active dhcpcd)" == "inactive" ]; then
	echo -e "${GREEN}[+] ${BLUE}Starting DHCP service${WHITE}"
	sudo service dhcpcd start
else
	echo -e "${YELLOW}[-] ${BLUE}DHCP service already running${WHITE}"
fi
if [ "$(systemctl is-enabled dhcpcd)" == "disabled" ]; then
	echo -e "${GREEN}[+] ${BLUE}Enabling DHCP service${WHITE}"
	sudo systemctl enable dhcpcd 2>/dev/null
else
	echo -e "${YELLOW}[-] ${BLUE}DHCP service already enabled${WHITE}"
fi

# Adding the static ip config only if an entry doesn't already exist
IS_IP_STATIC=$(grep -q '#Static IP for PtokaX' /etc/dhcpcd.conf && echo true || echo false)
if [ "$IS_IP_STATIC" == "false" ]; then
	echo -e "${GREEN}[+] ${BLUE}Making RPi's IP address static${WHITE}"

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

# Downloading other componenets
if [ ! -d ~/MetaHub ]; then
	echo -e "${GREEN}[+] ${BLUE}Cloning MetaHub repo${WHITE}"
	git clone --branch automate-hub-setup --single-branch https://github.com/proffapt/ptokax ~/MetaHub
	# TODO: Update it to the following after testing
	# git clone https://github.com/sheharyaar/ptokax
	sudo rm -rf ~/MetaHub/.git ~/MetaHub/README.md ~/MetaHub/ipofpi.sh ~/MetaHub/PtokaX
	# Making the scripts executable
	for file in ~/MetaHub/*; do 
		if [ -f "$file" ] && [ "${file##*.}" == "sh" ] && [ ! -x "$file" ]; then 
			chmod +x "$file"
		fi 
	done
else
	echo -e "${YELLOW}[-] ${BLUE}MetaHub repo already cloned${WHITE}"
fi

# Configuring PtokaX Aliases
ALIAS_CONFIGURED=$(grep -q 'source ~/MetaHub/ptokax-alias' ~/.bashrc && echo true || echo false)
if [ "$ALIAS_CONFIGURED" == "false" ]; then
	echo "source ~/MetaHub/ptokax-alias" >> ~/.bashrc
	source ~/.bashrc
fi

echo -e "${GREEN}[+] ${BLUE}Installing / Updating required packages${WHITE}"
# Curl for downloading source code
# mysql - required for scripts
# LUA 5.2.2 - Latest lua not installed as scripts are in lua version 5.2.2
sudo apt install -y curl liblua5.2-dev make g++ zlib1g-dev libtinyxml-dev default-libmysqlclient-dev lua-sql-mysql libcap2-bin
sudo apt autoremove -y

# Getting the PtokaX source code
if [ ! -d ~/MetaHub/PtokaX ]; then  
	# Download PtokaX source code
	echo -e "${GREEN}[+] ${BLUE}Downloading PtokaX source-code${WHITE}"
	curl -L -s https://github.com/sheharyaar/ptokax/releases/download/latest/ptokax-0.5.2.2-src.tgz -o ~/MetaHub/ptokax-0.5.2.2-src.tgz
	# Extract the archive
	echo -e "${GREEN}[+] ${BLUE}Extracting PtokaX source-code${WHITE}"
	tar -xf ~/MetaHub/ptokax-0.5.2.2-src.tgz -C ~/MetaHub
	rm -f ~/MetaHub/ptokax-0.5.2.2-src.tgz
else
	echo -e "${YELLOW}[-] ${BLUE}Extracted PtokaX source-code already exist${WHITE}"
fi

# Compiling PtokaX
if [ ! -f ~/MetaHub/PtokaX/skein/skein.a ]; then
	echo -e "${GREEN}[+] ${BLUE}Compiling PtokaX${WHITE}"
	cd ~/MetaHub/PtokaX/ || (echo "cd to ~/MetaHub/PtokaX failed" && exit)
	for dir in "obj" "skein/obj"; do
		if [ ! -d "$dir" ]; then
			mkdir obj skein/obj
		fi
	done
	make -f makefile-mysql lua52
	cd ~ || (echo "cd to ~ failed" && exit)
else
	echo -e "${YELLOW}[-] ${BLUE}PtokaX already compiled${WHITE}"
fi

# Installing PtokaX
if [ ! -f /usr/local/bin/PtokaX ]; then
	echo -e "${GREEN}[+] ${BLUE}Installing PtokaX${WHITE}"
	cd ~/MetaHub/PtokaX/ || (echo "cd to PtokaX failed" && exit)
	sudo make install
	cd ~ || (echo "cd to ~ failed" && exit)
else
	echo -e "${YELLOW}[-] ${BLUE}PtokaX already installed${WHITE}"
fi

# TODO: Do something with next 2 lines to autmate the handling of every case possible
echo -e "${GREEN}[+] ${BLUE}Setting up PtokaX${WHITE}"
cd ~/MetaHub/PtokaX/ || (echo "cd to PtokaX failed" && exit)
./PtokaX -m
cd ~ || (echo "cd to ~ failed" && exit)

# Getting PtokaX scripts
if [ -z "$(ls -A ~/MetaHub/PtokaX/scripts)" ]; then
	echo -e "${GREEN}[+] ${BLUE}Downloading Hit Hi Fit Hai scripts${WHITE}"
	git clone https://github.com/sheharyaar/ptokax-scripts ~/MetaHub/PtokaX/scripts/
else
	echo -e "${YELLOW}[-] ${BLUE}Hit Hi Fit Hai scripts already exist${WHITE}"
fi

# Editing SettingDefaults.h file
IS_BUG_FIXED_1=$(grep -q "${RASPI_IP%%/*}" ~/MetaHub/PtokaX/core/SettingDefaults.h && echo true || echo false)
if [ "$IS_BUG_FIXED_1" == "false" ]; then
	echo -e "${GREEN}[+] ${BLUE}Modifying ${YELLOW}~/MetaHub/PtokaX/core/SettingDefaults.h${WHITE}"
	sed -i "s/.*HUB_NAME/    \"MetaHub\", \/\/HUB_NAME/" ~/MetaHub/PtokaX/core/SettingDefaults.h
	sed -i "s/.*HUB_ADDRESS/    \"${RASPI_IP%%/*}\", \/\/HUB_ADDRESS/" ~/MetaHub/PtokaX/core/SettingDefaults.h
	sed -i "s/.*REDIRECT_ADDRESS/    \"${RASPI_IP%%/*}:411\", \/\/REDIRECT_ADDRESS/" ~/MetaHub/PtokaX/core/SettingDefaults.h
else
	echo -e "${YELLOW}[-] ${BLUE}BUG is already fixed in ${YELLOW}~/MetaHub/PtokaX/core/SettingDefaults.h${WHITE}"
fi

# Editing Settings.h file
IS_BUG_FIXED_2=$(grep -q "${RASPI_IP%%/*}:411" ~/MetaHub/PtokaX/cfg/Settings.pxt && echo true || echo false)
if [ "$IS_BUG_FIXED_2" == "false" ]; then
	echo -e "${GREEN}[+] ${BLUE}Modifying ${YELLOW}~/MetaHub/PtokaX/cfg/Settings.pxt${WHITE}"
	sed -i "s/.*HubName.*/HubName        =       MetaHub/" ~/MetaHub/PtokaX/cfg/Settings.pxt
	sed -i "s/.*HubAddress.*/HubAddress     =       ${RASPI_IP%%/*}/" ~/MetaHub/PtokaX/cfg/Settings.pxt
	sed -i "s/.*RedirectAddress.*/RedirectAddress        =       ${RASPI_IP%%/*}:411/" ~/MetaHub/PtokaX/cfg/Settings.pxt
else
	echo -e "${YELLOW}[-] ${BLUE}BUG is already fixed in ${YELLOW}~/MetaHub/PtokaX/cfg/Settings.pxt${WHITE}"
fi

if [ ! -f /etc/systemd/system/ptokax.service ]; then
	echo -e "${GREEN}[+] ${BLUE}Creating PtokaX service${WHITE}"
	chmod 644 ~/MetaHub/systemd/ptokax.service
	sudo cp ~/MetaHub/systemd/ptokax.service /etc/systemd/system/
	sudo chmod 777 /etc/systemd/system/ptokax.service
	sudo systemctl daemon-reload
else
	echo -e "${YELLOW}[-] ${BLUE}PtokaX service already created${WHITE}"
fi
if [ "$(systemctl is-enabled ptokax)" == "disabled" ]; then
	echo -e "${GREEN}[+] ${BLUE}Enabling PtokaX service${WHITE}"
	sudo systemctl enable ptokax
else
	echo -e "${YELLOW}[-] ${BLUE}PtokaX service already enabled${WHITE}"
fi

echo -e "${RED}[*] ${BLUE}Run PtokaX server using ${ORANGE}ptokax.start ${WHITE}"
