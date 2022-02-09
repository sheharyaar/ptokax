Raspberry Pi 4 Model B Rev 1.1 ( 2 GB ) \
OS : Rasbian 64 Bit

## Installing Ptokax on Raspberry Pi

To install PtokaX on raspberry Pi run the following commands in sequence :
```bash
cd ~
curl https://github.com/sheharyaar/ptokax/blob/main/ptokax-setup.sh -L -o ptokax-setup.sh
chmod +x ptokax-setup.sh
sudo ./ptokax-setup.sh
```

To run Ptokax server just run
```bash
./ptokax-start.sh
```

To stop PtokaX server just run
```bash
./ptokax-stop.sh
```

# Changes made to Ptokax 0.5.2.2


