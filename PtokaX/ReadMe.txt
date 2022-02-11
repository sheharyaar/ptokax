PtokaX 0.5.2.2
---------------

This is PtokaX version without gui able to run from console or as windows service.
You can use for that version config files from gui version of PtokaX or you can create own config files, see examples in cfg.example directory.

Command line commands available:
-c <configdir>		- absolute path to PtokaX config directory (where will PtokaX have cfg, logs, scripts and texts directories).
-d					- run as daemon.
-h					- show help.
-m					- show PtokaX configuration menu.
-p <pidfile>		- path with filename where PtokaX PID will be stored.
-v					- show PtokaX version with build date and time.
/generatexmllanguage	- generate english language example file.

In case when that version not start for you check logs directory, in most cases it log problem.

When no config dir is specified then PtokaX running from console use actual directory, PtokaX running as daemon use current_user_home/.PtokaX !

Homepage: http://www.PtokaX.org

PtokaX Forum: http://forum.PtokaX.org

PtokaX Wiki: http://wiki.PtokaX.org

Please report all bugs on forum or to PPK@PtokaX.org or to me (PPK) on 'PtokaX Dev Hub' dchub://PXdev.CzDC.org:2002

Note: This version have Lua scripting interface incompatible with all scripts for 0.3.6.0c and older !
Note: IP to Country database files are available on http://software77.net/geo-ip/ for IPv4 you need 'IPV4 CSV (gz)' and for IPV6 'IPV6 Range (gz)', unpack them to PtokaX/cfg directory.

Latest PtokaX changelog: http://wiki.PtokaX.org/doku.php?id=changelog