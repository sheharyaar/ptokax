# PtokaX Service Management

This directory contains everything related to management of ptokax as a service.<br>
It has the following structure with their description in brief:

```graphql
.
├── README.md
├── ptokax-alias {Aliases to service management scripts}
├── scripts {Contains scripts for service management}
│   ├── ptokax-config.sh
│   ├── ptokax-start.sh
│   ├── ptokax-status.sh
│   └── ptokax-stop.sh
└── systemd
    ├── ptokax-network-check.py {Makes sure internet connection is established then runs the setup script}
    ├── ptokax-startup-service.sh {Executes py script and starts ptokax server}
    └── ptokax.service {Service file, to run on every reboot}
```
