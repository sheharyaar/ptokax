#!/bin/bash

home=$(echo $HOME)

cd $home

printf "\n\n[log] Downloading start and stop scripts\n\n"

curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-start.sh -L -o ./ptokax-start.sh
chmod +x ./ptokax-start.sh

curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-stop.sh -L -o ./ptokax-stop.sh
chmod +x ./ptokax-stop.sh

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
git clone https://github.com/HiT-Hi-FiT-Hai/ptokax-scripts
cp ptokax-scripts/* PtokaX/scripts/ -rf

printf "\n\nRun Ptokax server using \"sudo ~/ptokax-start.sh\" \n"
