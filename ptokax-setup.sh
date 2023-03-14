#!/bin/bash

set -eou pipefail

# Colors - Obvious no?
export TERM=xterm
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

# Making the all the scripts executable
for file in /home/pi/MetaHub/*; do 
	if [ -f "$file" ] && [ "${file##*.}" == "sh" ] && [ ! -x "$file" ]; then 
		echo -e "${GREEN}[+] ${BLUE}Making $file executable${WHITE}"
		chmod +x "$file"
	fi 
done

# Configuring PtokaX Aliases
ALIAS_CONFIGURED=$(grep -q 'source /home/pi/MetaHub/psm/ptokax-alias' /home/pi/.bashrc && echo true || echo false)
if [ "$ALIAS_CONFIGURED" == "false" ]; then
	echo "source /home/pi/MetaHub/psm/ptokax-alias" >> /home/pi/.bashrc
	source /home/pi/.bashrc
fi

echo -e "${GREEN}[+] ${BLUE}Installing / Updating required packages${WHITE}"
# Curl for downloading source code
# mysql - required for scripts
# LUA 5.2.2 - Latest lua not installed as scripts are in lua version 5.2.2
sudo apt-get update
sudo apt-get install -y curl liblua5.2-dev make g++ zlib1g-dev libtinyxml-dev default-libmysqlclient-dev lua-sql-mysql libcap2-bin
sudo apt-get autoremove -y

# Editing SettingDefaults.h file
IS_BUG_FIXED_1=$(grep -q "${RASPI_IP%%/*}" /home/pi/MetaHub/PtokaX/core/SettingDefaults.h && echo true || echo false)
if [ "$IS_BUG_FIXED_1" == "false" ]; then
	echo -e "${GREEN}[+] ${BLUE}Modifying ${YELLOW}/home/pi/MetaHub/PtokaX/core/SettingDefaults.h${WHITE}"
	sed -i "s/.*HUB_NAME/    \"MetaHub\", \/\/HUB_NAME/" /home/pi/MetaHub/PtokaX/core/SettingDefaults.h
	sed -i "s/.*HUB_ADDRESS/    \"${RASPI_IP%%/*}\", \/\/HUB_ADDRESS/" /home/pi/MetaHub/PtokaX/core/SettingDefaults.h
	sed -i "s/.*REDIRECT_ADDRESS/    \"${RASPI_IP%%/*}:411\", \/\/REDIRECT_ADDRESS/" /home/pi/MetaHub/PtokaX/core/SettingDefaults.h
else
	echo -e "${YELLOW}[-] ${BLUE}BUG is already fixed in ${YELLOW}/home/pi/MetaHub/PtokaX/core/SettingDefaults.h${WHITE}"
fi

# Editing Settings.h file
IS_BUG_FIXED_2=$(grep -q "${RASPI_IP%%/*}:411" /home/pi/MetaHub/PtokaX/cfg/Settings.pxt && echo true || echo false)
if [ "$IS_BUG_FIXED_2" == "false" ]; then
	echo -e "${GREEN}[+] ${BLUE}Modifying ${YELLOW}/home/pi/MetaHub/PtokaX/cfg/Settings.pxt${WHITE}"
	sed -i "s/.*HubName.*/#HubName        =       MetaHub/" /home/pi/MetaHub/PtokaX/cfg/Settings.pxt
	sed -i "s/.*HubAddress.*/#HubAddress     =       ${RASPI_IP%%/*}/" /home/pi/MetaHub/PtokaX/cfg/Settings.pxt
	sed -i "s/.*RedirectAddress.*/#RedirectAddress        =       ${RASPI_IP%%/*}:411/" /home/pi/MetaHub/PtokaX/cfg/Settings.pxt
else
	echo -e "${YELLOW}[-] ${BLUE}BUG is already fixed in ${YELLOW}/home/pi/MetaHub/PtokaX/cfg/Settings.pxt${WHITE}"
fi

# Updating MOTD with the IP of PI
IS_IP_UPDATED=$(grep -q "${RASPI_IP%%/*}" /home/pi/MetaHub/PtokaX/cfg/Motd.txt && echo true || echo false)
if [ "$IS_IP_UPDATED" == "false" ]; then
        echo -e "${GREEN}[+] ${BLUE}Modifying ${YELLOW}/home/pi/MetaHub/PtokaX/cfg/Motd.txt${WHITE}"
        sed -i "s/.*Hub Address.*/          Hub Address        -    ${RASPI_IP%%/*}/" /home/pi/MetaHub/PtokaX/cfg/Motd.txt
else
        echo -e "${YELLOW}[-] ${BLUE}IP is already updated in ${YELLOW}/home/pi/MetaHub/PtokaX/cfg/Motd.txt${WHITE}"
fi

# Compiling PtokaX
if [ ! -f /home/pi/MetaHub/PtokaX/skein/skein.a ]; then
	echo -e "${GREEN}[+] ${BLUE}Compiling PtokaX${WHITE}"
	cd /home/pi/MetaHub/PtokaX/ || (echo "cd to /home/pi/MetaHub/PtokaX failed" && exit)
	for dir in "obj" "skein/obj"; do
		if [ ! -d "$dir" ]; then
			mkdir obj skein/obj
		fi
	done
	make -f makefile-mysql lua52
	cd ~ || (echo "cd to ~ failed" && exit)
	sudo rm -f /usr/local/bin/PtokaX
else
	echo -e "${YELLOW}[-] ${BLUE}PtokaX already compiled${WHITE}"
fi

# Installing PtokaX
if [ ! -f /usr/local/bin/PtokaX ]; then
	echo -e "${GREEN}[+] ${BLUE}Installing PtokaX${WHITE}"
	cd /home/pi/MetaHub/PtokaX/ || (echo "cd to /home/pi/MetaHub/PtokaX failed" && exit)
	sudo make install
	cd ~ || (echo "cd to ~ failed" && exit)
else
	echo -e "${YELLOW}[-] ${BLUE}PtokaX already installed${WHITE}"
fi

# Getting PtokaX scripts
if [ ! -d /home/pi/MetaHub/PtokaX/scripts ]; then
	echo -e "${GREEN}[+] ${BLUE}Downloading Hit Hi Fit Hai scripts${WHITE}"
	git clone https://github.com/sheharyaar/ptokax-scripts /home/pi/MetaHub/PtokaX/scripts/
else
	echo -e "${YELLOW}[-] ${BLUE}Hit Hi Fit Hai scripts already exist${WHITE}"
fi

# Handling PtokaX service
if [ ! -f /etc/systemd/system/ptokax.service ]; then
	echo -e "${GREEN}[+] ${BLUE}Creating PtokaX service${WHITE}"
	chmod 644 /home/pi/MetaHub/psm/systemd/ptokax.service
	sudo cp /home/pi/MetaHub/psm/systemd/ptokax.service /etc/systemd/system/
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
