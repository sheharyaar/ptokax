Raspberry Pi 4 Model B Rev 1.1 ( 2 GB ) \
OS : Rasbian 64 Bit

# Installing Ptokax

- Installing Prerequisites : 

```bash
# Install g++, zlib, tinyxml
sudo apt install make g++ zlib1g-dev libtinyxml-dev

# Install lua 5.2.2
# Latest lua not installed as scripts are in lua version 5.2.2
sudo apt install liblua5.2-dev

# Install mysql - needed for scripts
sudo apt-get install libmysqlclient-dev
```

- Download PtokaX source code - 
`wget https://github.com/sheharyaar/ptokax/releases/download/latest/ptokax-0.5.2.2-src.tgz`

- Extract the archive -
`tar -xf ptokax-0.5.2.2-src.tgz`

- Import Hit-Hi-Fit-Hai scripts : 

```bash
git clone https://github.com/HiT-Hi-FiT-Hai/ptokax-scripts
cp ptokax-scripts/* PtokaX/scripts/ -rf
```

- Make the program - 
```bash
cd PtokaX
make -f makefile-mysql lua52
```

- Setup privileges in order to allow ptokax to run on port 411 (default port)
```bash
sudo apt install libcap2-bin
sudo make install
```

- Run ptokax using sudo to allow it to bind to port - `sudo ./PtokaX`

# Changes made to Ptokax 0.5.2.2


