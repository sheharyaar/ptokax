The PtokaX server runs in background using a systemd config. The commands to configure the script
```sh
//To enable the script
sudo systemctl enable ./ptokax.service

//To start the script
sudo systemctl start ./ptokax.service


//To check the status of the script
sudo systemctl status ./ptokax.service
```


# TODO: 
- Improve ptokax-dhcp.service
- Complete ptokax-ip.py
- Create a master script or improve the existing script to start the systemd scripts everywhere