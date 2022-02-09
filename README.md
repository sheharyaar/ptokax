Raspberry Pi 4 Model B Rev 1.1 ( 2 GB ) \
OS : Rasbian 64 Bit

## Installing Ptokax on Raspberry Pi

To install PtokaX on raspberry Pi run the following commands in sequence :
```bash
cd ~
curl https://github.com/sheharyaar/ptokax/releases/download/latest/setup.sh -L -o ptokax-setup.sh
chmod +x ptokax-setup.sh
sudo ./ptokax-setup.sh
```

## Importing Hit-Hi-Fit-Hai scripts

```bash
cd ~
git clone https://github.com/HiT-Hi-FiT-Hai/ptokax-scripts
cp ptokax-scripts/* PtokaX/scripts/ -rf
```

# Changes made to Ptokax 0.5.2.2


