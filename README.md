Raspberry Pi 4 Model B Rev 1.1 ( 2 GB ) \
OS : Rasbian 64 Bit

- [Searching the Pi and SSH](#searching-the-pi-and-ssh)
- [Installing Ptokax on Raspberry Pi](#installing-ptokax-on-raspberry-pi)
    - [Automatic Installation](#automatic-installation)
    - [Manual Installation](#manual-installation)
    - [BUG-FIX: Ban followed by wrong redirection](#bug-fix-ban-followed-by-wrong-redirection)
- [Running Scripts](#running-scripts)
    - Changing the Message of the Day
    - Changing the Share limit
- [Changes Made to HHFH Version](#changes-made-to-ptokax-hhfh-version)

# Searching the Pi and SSH

1. Install the following dependencies based on your system:
   - Linux
      - `nmap`
   - MacOS
      - `nmap`
      - `iproute2mac`
      
    For Debian/Ubuntu : `sudo apt update && sudo apt install -y nmap`<br>
    For MacOS using brew : `brew install nmap iproute2mac`<br>
2. Make sure you are connected to the same ethernet subnetwork as that of pi before you start the search for its ip.
3. Follow either one of the following methods to get the ip:
   - [Automatic Searching](#automatic-searching)
   - [Manual Searching](#manual-searching)
4. Now ssh to the pi : `ssh pi@IP`. If it is a new install, then the password will be `raspberrypi`.

### Automatic Searching

Run the follwoing set of commands to get the `ipofpi.sh` script and execute it.
```bash
git clone https://github.com/proffapt/ptokax --depth 1 --branch automate-hub-setup --single-branch MetaHub
mv MetaHub/ipofpi.sh ./
chmod +x ipofpi.sh
rm -rf MetaHub
./ipofpi.sh
```
<!-- Todo: Change the above with below
```bash
git clone https://github.com/sheharyaar/ptokax --depth 1 --branch main --single-branch MetaHub
mv MetaHub/ipofpi.sh ./
chmod +x ipofpi.sh
rm -rf MetaHub
./ipofpi.sh
```
-->
   
   > **Note** All this drama needs to be done because https://raw.githubusercontent.com is blocked on campus network :)
  
### Manual Searching

- Check your ethernet interface name by running : `ip addr`. <br>
In my case it's `eth0`.

- Run the following command to get your subnet IP and replace `eth0` with your interface name : 
  ```bash
  ip route | grep link | grep eth0 | cut -d ' ' -f 1
  ```
- If you are not on the same subnet then get the subnet address from a friend on the same subnet as in the above step.
- Now to find Pi run (Replace `10.112.5.0/24` with yout subnet address ) :
  ```bash
  sudo nmap -sS -O -p22 10.112.5.0/24 -oG result
  ```
  This will create a file named `result`.
  
Now view the contents of the file, it will be similar to this:
```bash
# Nmap 7.92 scan initiated Fri Feb 25 00:55:18 2022 as: nmap -sS -O -p22 -oG testing 10.112.5.0/24
Host: 10.112.5.2 ()	Status: Up
Host: 10.112.5.2 ()	Ports: 22/closed/tcp//ssh///
Host: 10.112.5.32 ()	Status: Up
Host: 10.112.5.32 ()	Ports: 22/filtered/tcp//ssh///
Host: 10.112.5.123 ()	Status: Up
Host: 10.112.5.123 ()	Ports: 22/closed/tcp//ssh///
Host: 10.112.5.167 ()	Status: Up
Host: 10.112.5.167 ()	Ports: 22/open/tcp//ssh///	OS: Linux 4.15 - 5.6	Seq Index: 260	IP ID Seq: All zeros
# Nmap done at Fri Feb 25 00:55:25 2022 -- 256 IP addresses (4 hosts up) scanned in 7.22 seconds
```
In this case, the status is **Up**, the port is **open** and the OS is **Linux**. If there are more than one devices, then try with all of them.
```bash
Host: 10.112.5.167 ()	Status: Up
Host: 10.112.5.167 ()	Ports: 22/open/tcp//ssh///	OS: Linux 4.15 - 5.6	Seq Index: 260	IP ID Seq: All zeros
```

# Installing Ptokax on Raspberry Pi

## Automatic Installation

First __clone__ the repository in a _structured manner_ and _remove unnecessary files_, then execute the setup script using the following set of commands.
```bash
git clone https://github.com/proffapt/ptokax --depth 1 --branch automate-hub-setup --single-branch /home/pi/MetaHub
sudo rm -rf /home/pi/MetaHub/.git /home/pi/MetaHub/README.md /home/pi/MetaHub/ipofpi.sh
chmod +x /home/pi/MetaHub/ptokax-setup.sh
./MetaHub/ptokax-setup.sh
source ~/.bashrc
```
<!-- Todo: Change the above with below
```bash
git clone https://github.com/sheharyaar/ptokax --depth 1 --branch main --single-branch /home/pi/MetaHub
sudo rm -rf /home/pi/MetaHub/.git /home/pi/MetaHub/README.md /home/pi/MetaHub/ipofpi.sh
chmod +x /home/pi/MetaHub/ptokax-setup.sh
./MetaHub/ptokax-setup.sh
source ~/.bashrc
```
-->

   > **Note** All this drama needs to be done because https://raw.githubusercontent.com is blocked on campus network :)
  
And, after completing the above steps use the below commands to achieve the specified tasks:
- To start the Ptokax server: `ptokax.start`
- To stop the PtokaX server: `ptokax.stop`
- To check the status of PtokaX server: `ptokax.status`
- To configure the PtokaX server: `ptokax.config`

## Manual Installation

The first step involves getting the IP address of the Raspberry Pi assigned to it.

1) Check if dhcp service is running : `sudo service dhcpcd status` <br>
In case it’s not, activate DHCPCD as follows:
    ```bash
    sudo service dhcpcd start
    sudo systemctl enable dhcpcd
    ```

2) Now check the **IP Address of the PI** : `ip addr`
    ```bash
    2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
        link/ether dc:a6:32:4c:e7:3a brd ff:ff:ff:ff:ff:ff
        inet 10.112.5.167/24 brd 10.112.5.255 scope global noprefixroute eth0
           valid_lft forever preferred_lft forever
        inet6 fe80::4e04:ae25:cae7:adeb/64 scope link 
           valid_lft forever preferred_lft forever
    ```
    Here the line : `inet 10.112.5.167/24 brd 10.112.5.255 scope global noprefixroute eth0` gives the IP address.<br>
    In this case it is `10.112.5.167`.

3) Check the **GATEWAY_ADDRESS** : `ip route | grep default`. <br>
This will give output similar to : `default via 10.112.5.2 dev eth0 src 10.112.5.167 metric 202` <br>
In this case the Gateway Address is : `10.112.5.2` <br>

4) Check the **DNS_ADDRESS** : `cat /etc/resolv.conf`. This gives : 
    ```bash
    # Generated by resolvconf
    nameserver 127.0.0.53
    ```
    The DNS Address is : `127.0.0.53` or the first IP Address.

5) Edit the dhcp config file : `sudo nano /etc/dhcpcd.conf` <br>
Add to the end of the config. If it is already there and commented out using # then uncomment and edit.
    ```bash
    interface eth0
    static ip_address=IP_ADDRESS_TO_BE_ASSIGNED_TO_PI/24
    static routers=GATEWAY_ADDRESS
    static domain_name_servers=DNS_ADDRESS
    ```

## BUG-FIX: Ban followed by wrong redirection

> **Note** This bug exists only if the setup is a fresh install otherwise this has already been addressed in [3ae699421373c49c6a640e927222a76f68f07835](https://github.com/sheharyaar/ptokax/commit/3ae699421373c49c6a640e927222a76f68f07835) - refer it for more information as well.

In this bug users are sometimes redirected to the wrong hub address and are banned. To fix this, change `REDIRECT_ADDRESS`, `HUB_NAME` & `HUB_ADDRESS` - in the following files as shown.

- PtokaX/core/SettingDefaults.h
  ```bash
  const char* SetTxtDef[] = {
      "MetaHub", //HUB_NAME
      "Admin", //ADMIN_NICK
      "10.112.5.167", //HUB_ADDRESS
      "1209;411", //TCP_PORTS
      "0", //UDP_PORT
      "", //HUB_DESCRIPTION
      "10.112.5.167:411", //REDIRECT_ADDRESS
      "reg.hublist.org;serv.hubs-list.com;hublist.cz;hublist.dreamland-net.eu;allhublista.myip.hu;publichublist-nl.no-ip.org;reg.hublist.dk;hublist.te-home.net;dc.gwhublist.com", //REGISTER_SERVERS
      "Sorry, this hub is only for registered users.", //REG_ONLY_MSG
      "", //REG_ONLY_REDIR_ADDRESS
  ```

- PtokaX/cfg.example/Settings.pxt

  ```bash
  # Hub name. Minimal length 1, maximal length 256. $ and | is not allowed
  #HubName	=	MetaHub
  # Admin nick. Minimal length 1. Maximal length 64. $, | and space is not allowed
  #AdminNick	=	Admin
  # Hub address. Minimal length 1. Maximal length 256. $ and | is not allowed
  #HubAddress	=	10.112.5.167
  # TCP ports. Minimal length 1. Maximal length 64
  #TCPPorts	=	1209;411
  # UDP port. Minimal length 1. Maximal length 5
  #UDPPort	=	0
  # Hub description. Maximal length 256. $ and | is not allowed
  #HubDescription	=	
  # Main redirect address. Maximal length 256. | is not allowed
  #RedirectAddress	=	10.112.5.167:411
  # Hublist register servers. Maximal length 1024
  ```
  
## Running Scripts

Repositories with all the PtokaX Scripts : [https://github.com/sheharyaar/ptokax-scripts](https://github.com/sheharyaar/ptokax-scripts)

> **Warning**: Make sure to stop ptokax first and then change settings otherwise default settings are stored.

- Messsage of The Day : Edit `~/PtokaX/cfg/Motd.txt`
- Change Share Limit : Edit `~/PtokaX/scripts/external/restrict/share.lua`
  ```lua
  -- change the 3rd value in units of GiB ( 90 means 90 GiB )
  local sAllowedProfiles, iBanTime, iShareLimit, iDivisionFactor = "01236", 6, 0, ( 2^10 )^3
  ```
  and then start/restart the following scripts using the !startscript/!restartscript command _after logging into the DC_ using any client (LinuxDC++ preferred on Linux and DC++ on windows)

  ```
  !startscript startup.lua
  !startscript restrictions.lua
  ```

## Changes made to Ptokax HHFH version

- Fixed errors in compilation

```console
/home/pi/PtokaX/core/SettingManager.cpp: In member function ‘void SettingManager::Save()’:
/home/pi/PtokaX/core/SettingManager.cpp:507:25: error: ISO C++ forbids comparison between pointer and integer [-fpermissive]
  507 |      if(SetBoolCom[szi] != '\0') {
      |         ~~~~~~~~~~~~~~~~^~~~~~~
/home/pi/PtokaX/core/SettingManager.cpp:530:26: error: ISO C++ forbids comparison between pointer and integer [-fpermissive]
  530 |      if(SetShortCom[szi] != '\0') {
      |         ~~~~~~~~~~~~~~~~~^~~~~~~
/home/pi/PtokaX/core/SettingManager.cpp:553:24: error: ISO C++ forbids comparison between pointer and integer [-fpermissive]
  553 |      if(SetTxtCom[szi] != '\0') {
      |         ~~~~~~~~~~~~~~~^~~~~~~
/home/pi/PtokaX/core/SettingManager.cpp: In member function ‘void SettingManager::SetText(size_t, const char*, size_t)’:
/home/pi/PtokaX/core/SettingManager.cpp:1112:41: warning: this statement may fall through [-Wimplicit-fallthrough=]
 1112 |             if(szLen == 0 || szLen > 64 || strpbrk(sTxt, " $|") != NULL) {
      |                                         ^
/home/pi/PtokaX/core/SettingManager.cpp:1115:9: note: here
 1115 |         case SETTXT_TCP_PORTS:
      |         ^~~~
make: *** [makefile:340: /home/pi/PtokaX/obj/SettingManager.o] Error 1
```

- Created scripts to setup, start and stop ptokax server.
- Created script to check the status of ptokax server.
- Created service for ptokax to be executed when the pi is restarted.

