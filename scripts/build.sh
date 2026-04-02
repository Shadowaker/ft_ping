#!/bin/bash
# DO NOT RUN BASH SCRIPT WITHOUT CHECKING
# This script will permit the program to run without sudo.
# This script needs sudo to work properly.

echo "Installing libcap dependency"

# install libcap dependency
apt install libcap2-bin -y

# setcap
setcap cap_net_raw+ep ../ft_ping 2>/dev/null

