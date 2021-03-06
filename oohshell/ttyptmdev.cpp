/**
# Copyright (C) 2007-2015 s(mariuschincisan@gmail.com) - coinscode.com - N/A
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


#define _XOPEN_SOURCE 600

#include <os.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stropts.h>

#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>

#include "ttyptmdev.h"

ttyio::ttyio():_master(0),_slave(0),_bashid(0)
{
    std::cout << getpid() << "TTY {{\n";
}

ttyio::~ttyio()
{
    closems();
    std::cout << getpid() << "TTY }}\n";
}


int ttyio::read(char* outbuff, int cap)
{
    return ::read(_master, outbuff, cap);
}


int ttyio::write(const char* intbuff, int chars)
{
    return ::write(_master, intbuff, chars);
}


int ttyio::grant(uid_t u, gid_t g, mode_t mode)
{
    return 0; // when ran as root damages the /dev/pty and you cannot open a console, then
}


void ttyio::closems()
{
    if(_master){
        write("exit\r\n",6);
        usleep(500000);
        ::close(_master);
        Proco.killit(_bashid);
    }
    if(_slave){
        ::close(_slave);
    }
    _master=_slave=0;
}

void ttyio::reopen()
{

}

int ttyio::opendev()
{
    _m="/dev/ptmx";

    assert(_master==0);
    assert(_slave==0);

    if (::access(_m.c_str(),0)!=0)
    {
        std::cout <<"cannot open: " << "/dev/ptmx\n";
        return -1;
    }

	if ((_master = ::open("/dev/ptmx", O_RDWR|O_NOCTTY)) < 0)
	{
		std:: cout << __FILE__ << "," <<__LINE__ << strerror(errno) << "\n";
		return -1;
	}
	std::cout  <<"open master: " << _master << "\n";
	grantpt(_master);
	::unlockpt(_master);

    struct winsize ws;
    ioctl (_master, TIOCGWINSZ, &ws);
    ws.ws_col=80;
    ws.ws_row=80;
    ws.ws_xpixel=1;
    ws.ws_ypixel=1;
    ioctl(_master, TIOCSWINSZ, &ws);


    _s=::ptsname(_master);

    _bashid = Proco.forkit();
    if(_bashid==0) //we are in child
    {
        _childproc();
    }
    return _bashid;
}

void ttyio::_childproc()
{
    ::close(_master);
    _master = 0;

    ::setsid();
    if ((_slave = ::open(_s.c_str(), O_RDWR|O_NOCTTY)) < 0)
	{
        std:: cout << __FILE__ << "," <<__LINE__ << strerror(errno) << "\n";
		exit(0);
	}
	::ioctl(_slave, I_PUSH, "ptem");
	::ioctl(_slave, I_PUSH, "ldterm");
    ::ioctl(0, TIOCSCTTY, 0);
	::dup2 (_slave, 0);		/* stdin */
	::dup2 (_slave, 1);		/* stdout */
	::dup2 (_slave, 2);		/* stderr */
	::system ("stty sane");
	::execl ("/bin/bash", "bash", (char *) 0);
	std:: cout << "  /bin/bash\n";
	Proco.exitproc(0,"/bin/bash exiting");

}




