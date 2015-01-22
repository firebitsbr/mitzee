#!/bin/bash
sudo iptables -t nat -D PREROUTING 1
sudo iptables -t nat -D PREROUTING 1
sudo iptables -t nat -D PREROUTING 1
sudo iptables -t nat -D PREROUTING 1
sudo iptables -t nat -D PREROUTING 1

sudo iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-port 8053
./mariuz
pkill mariuz
sudo iptables -t nat -D PREROUTING 1

