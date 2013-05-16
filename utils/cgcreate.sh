#!/usr/bin/env bash

cat cg.conf | sed "s/_USER_/$USER/" | sudo tee -a /etc/cgconfig.conf
sudo cgconfigparser -l /etc/cgconfig.conf

