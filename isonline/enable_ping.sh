#!/bin/sh
sysctl -w net.ipv4.ping_group_range="0 2000"
if [ $(id -u) -eq 0 ]; then
	echo "net.ipv4.ping_group_range=0 2000" >> /etc/sysctl.conf
	echo "net.ipv6.ping_group_range=0 2000" >> /etc/sysctl.conf
	sysctl -p
fi

