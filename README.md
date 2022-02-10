Raspberry Pi 4 Model B Rev 1.1 ( 2 GB ) \
OS : Rasbian 64 Bit

## Installing Ptokax on Raspberry Pi

To install PtokaX on raspberry Pi run the following commands in sequence :
```bash
cd ~
curl https://raw.githubusercontent.com/sheharyaar/ptokax/main/ptokax-setup.sh -L -o ptokax-setup.sh
chmod +x ptokax-setup.sh
./ptokax-setup.sh
```

To run Ptokax server just run
```bash
./ptokax-start.sh
```

To stop PtokaX server just run
```bash
./ptokax-stop.sh
```

To configure PtokaX server just run
```bash
./ptokax-config.sh
```

## Running Scripts

Make sure to stop ptokax first and then change settings otherwise default settings are stored.

- Messsage of The Day : Edit `~/PtokaX/cfg/Motd.txt`
- Change Share Limit : Edit `~/PtokaX/scripts/external/restrict/share.lua` 

```lua
-- change the 3rd value
local sAllowedProfiles, iBanTime, iShareLimit, iDivisionFactor = "01236", 6, 0, ( 2^10 )^3
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

- Created script to setup, start and stop ptokax server
