#!/usr/bin/env bash

# config
target="/etc/cgconfig.conf"

# parse template
data=$(cat cg.conf | sed "s/_USER_/$USER/")

# write config
if [ -e "$target" ] && grep -q -F "$data" "$target"; then
	echo "Skip existing config"
else
	echo "write config to $target"
	echo "$data" | sudo tee -a "$target" 1>/dev/null
fi

# load config
echo "Reload config"
sudo cgconfigparser -l "$target" 1>/dev/null

