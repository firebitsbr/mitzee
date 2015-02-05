/**
# Copyright (C) 2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - coinscode.com - N/A
#
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
*/


#include "theapp.h"
#include "config.h"
#include "message.h"
#include "watcher.h"
#include <minidb.h>
//-------------------------------------------------------------------------------
// redirect 53 to 5353 and open that one instead
// sudo iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-port 5353


bool              __alive=true;

//-------------------------------------------------------------------------------
int main()
{
    Conf            cfg("miki.conf");
    theapp          app;
    return app.run();
}

