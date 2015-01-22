/**
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
#
*/


#include <QProgressBar>
#include "dialog.h"
#include "udpsocks.h"


int Udper::discover(int& progress, Dialog* pd)
{
    if(_sock==0)
    {
        if(_create(_sock))
        {
            if(!_broad())
                return -1;
        }else
            return -1;
        std::cout << "openned        port:" <<    SRV_PORT << "\n";
    }
    if(_rock==0)
    {
        if(_create(_rock))
        {
            if(!_bind_to_addr(_rock, _srv_addr,INADDR_ANY, CLI_PORT))
                return -1;
        }else
            return -1;

        std::cout << "discoverring on port:" <<    CLI_PORT << "\n";
    }

    // broadcast on _sock
    struct sockaddr_in skaddr;

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = INADDR_BROADCAST;
    skaddr.sin_port = htons(SRV_PORT);


    ::sendto(_sock,PI_QUERY,::strlen(PI_QUERY),0,(struct sockaddr*) &skaddr,sizeof(skaddr));
    for(int k=0;k<256;++k)
    {
        if(k%8==0)
            ::sendto(_sock,PI_QUERY,::strlen(PI_QUERY),0,(struct sockaddr*) &skaddr,sizeof(skaddr));
        //_wait on _rock
        int n = _is_signaled(_rock);
        if(n==-1)
        {
            std::cout << "no events \n";
            break;
        }
        if(n>0)
        {
            char                buff[128];
            struct sockaddr_in  remote;
            socklen_t           len = sizeof(remote);
            n=::recvfrom(_rock,buff,sizeof(buff)-1,0,(struct sockaddr *)&remote,&len);
            if(n>0)
            {
                buff[n] = 0;
                if(_remips.find(buff)==_remips.end())
                {
                    _remips.insert(buff);
                }
            }
        }
        usleep(2048);
        ++progress;
        pd->ui->progressBar->setValue(progress);
    }

    return _remips.size();
}
