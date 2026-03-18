# Nice Pillz ZMK Config

## Overview
ZMK Configurations for the the NicePillz Board: https://github.com/nol00p/NicePillz

The layout is Linux/Gnome driven.

## Features supported
- [x] ZMK Studio 
- [x] Leader key
- [x] Home row mode
- [x] caps word
- [x] Helper.h
- [x] Tri State Layer
- [x] Macro
	- [x] with unicode support. 
###### Supported but not yet tested
- [ ] Combo

## Battery level Tweaks
the battery reporting is included so it can be visible in the os. 
bluetooth power, speed and timeour update too for better experience. 

## LED Behavior
the power led shows the power status of the Board. the ble led, shows when the board is connected to bluetooth.
Sleep modes: 
sleep (timer 5min): pwr led flashs, ble led off. 
Deep Sleep (timer 20min): both led off

## Sleep modes
Sleep, 5 min timer - any keys to resume
Deep Sleep, 20 min timer - press the esc key (0,0) to wake the board up.

## Extra

A quick shell script to get the battere state of the keyboard on the CLI.

```
!/bin/bash
# Compact colored battery display

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ $# -ne 1 ]; then
  echo "Usage: $0 MAC_ADDRESS"
  exit 1
fi

battery=$(bluetoothctl info "$1" | awk -F'[()]' '/Battery Percentage/ {print $2}')

if [ -z "$battery" ]; then
  echo -e "${RED}✗ No battery info${NC}"
  exit 1
fi

# Choose color based on battery level
if [ "$battery" -ge 70 ]; then
  color=$GREEN
  icon="🔋"
elif [ "$battery" -ge 30 ]; then
  color=$YELLOW
  icon="🔋"
else
  color=$RED
  icon="🪫"
fi

# Create simple bar
filled=$((battery / 5))
empty=$((20 - filled))
bar=""
for ((i = 0; i < filled; i++)); do bar+="█"; done
for ((i = 0; i < empty; i++)); do bar+="░"; done

echo -e "${icon}  ${color}${battery}%${NC} [${color}${bar}${NC}]"
```

## Credits
https://github.com/dcpedit/pillzmod

https://github.com/masters3d/zmk-config-pillzmod-nicenano

https://github.com/urob/zmk-leader-key

https://github.com/urob/zmk-helpers

