#!/bin/bash
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - getic.net - N/A
#           FOR HOME USE ONLY. For corporate  please contact me
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if [[ $1 == "--remove" ]];then
    sudo service pidiscd stop
    sleep 1
    sudo update-rc.d -f pidiscd remove
    exit
fi

if [[ -f /usr/bin/cmake && -f /usr/bin/make && -f /usr/bin/g++ && -f /usr/bin/g++ ]];then
    cmake .
    make clean
    make
fi

if [[ -f ./pidisc ]];then
	sudo cp ./pidisc /usr/bin
	sudo chmod +x /usr/bin/pidisc
else
	echo "pidisc executable was not found in current folder!!!"
	exit
fi

echo "Please enter the unique token you are querying the IP of this device. Up to 30 characters"
read NEW_TOKEN


sudo cp ./pidisc.sysV /etc/init.d/pidiscd
sudo sed -i "s/QUERY_TOKEN/$NEW_TOKEN/g" /etc/init.d/pidiscd
sudo chmod +x /etc/init.d/pidiscd
sudo touch /var/log/pidisc.log
sudo update-rc.d -f pidiscd remove
sleep 1
sudo update-rc.d pidiscd defaults
sleep 1
echo "service installed. Starting service"
sudo service pidiscd start
echo "checking service..."
sleep 1
check=$(ps ax | grep pidisc | grep -v grep)
if [[ $check =~ "pidisc l" ]];then
    echo "OK Service is running"
else
    echo "FAIL. Service did not start. Try to reboot!"
fi
echo "to uninstall the service run: ./install.sh --remove"


