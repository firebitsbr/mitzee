
```
    Oohshell, Remote Shell server for linux (V).
    Miki DNS spoofer
    Mitza Web crawler and indexer  (has a life demo)
    SOCKS4 SOCKS5 HTTP PROXY server (has a life demo)
    Pizu Web server with Squirel script server pages (for small systems)
    
```

[download-sourcecode](https://mitzee.googlecode.com/archive/08f54948f08661101d5649326e2077d4e7e7f12c.zip)



### Mitza web crawler ###
Life demo at: http://staging.mine.nu/?w=interior+design+
Video: https://www.youtube.com/watch?v=9WVLw6nlCak

  * OS: Linux
  * Code::Blocks project and cmake: CMakeLists.txt
  * Dependencies: libgnutls-openssl27
  * Dependencies: libpcrecpp0
  * Dependencies: libhtmlcxx
  * Dependencies: libhtmlcxx3
  * Dependencies: mysql-server
  * Dependencies: mysql-client
  * For the web part:
  * Dependencies: apache2
  * Dependencies: php5
  * Dependencies: libapache2-mod-php5
  * Dependencies: php5-mysql


### Pizu Web server ###

  * OS: Linux
  * C++ (stl), Dependencies (pthread, dl and mcosocks)
  * Code::Blocks project and cmake: CMakeLists.txt
  1. Multi-threaded, non-blocking sockets, using select
  1. Squirell script server pages.'http://www.squirrel-lang.org/'
  1. ssl support on server side
  1. plugins as so libraries.

demo: http://marius.mine.nu/


### Buflea proxy server ###
Life demo at:  http://screen.webhop.org
Video: https://www.youtube.com/watch?v=5g5luEPuWgc

  * OS: Linux
  * C++ (stl), Dependencies (pthread, dl and mcosocks)
  * Code::Blocks project and cmake: CMakeLists.txt
  * SOCKS4 5 HTTP, custom DNS, PASSTRU-(reverse/transparent), ssl


# BUFLEA #

Block diagram, I mean: `bloc daiagram`.


![https://mitzee.googlecode.com/git-history/fcf1aaeb527f00fea63fb5da85a627648cd4d08a/tuls/buflea-block.png](https://mitzee.googlecode.com/git-history/fcf1aaeb527f00fea63fb5da85a627648cd4d08a/tuls/buflea-block.png)


Flow... , Ai min: `flou daiagram`.


![https://mitzee.googlecode.com/git-history/fcf1aaeb527f00fea63fb5da85a627648cd4d08a/tuls/bufleaflow.png](https://mitzee.googlecode.com/git-history/fcf1aaeb527f00fea63fb5da85a627648cd4d08a/tuls/bufleaflow.png)


`Working modes`


![https://mitzee.googlecode.com/git-history/4bdcd2c7016d34606d5ed042b388c3bc3a6fac84/tuls/bufleamodes.png](https://mitzee.googlecode.com/git-history/4bdcd2c7016d34606d5ed042b388c3bc3a6fac84/tuls/bufleamodes.png)



### Miki DNS Spoof ###

  * OS: Linux
  * C++ (stl), Dependencies (pthread, dl and mcosocks)
  * Code::Blocks project and cmake: CMakeLists.txt
  * UDP only.

Forwards the DNS query to preconfigured DNS servers, see: nextdnss@miki.conf.
Traps the name-servers from [rules](rules.md) @ miki.conf from DNS replyes and
replaces the IP's accordingly.

If is used in tandem with
Buflea Proxy, (proxyip@@miki.conf is set) sends the IP for the domain/or-spoffed
the client queried to Buflea. A DNS-type-of socket on Buflea then would connect
the client to that IP.




## oohshell ##
### remote shell (client and server), (like telnetd) ###
  * Remote shell (open non-secured).
  * Added on Feb 27 2015
  * OS Linux (pty98)
  * C++
  * code blocks project cbp
  * ...work in progress...
  * 
The oohsheel is to be used instead dropbear or ssh on small devices on secure networks where dropbear and or ssh is hard to be ported. The oohshell uses only pthread library.
The system requires /dev/ptmx (UNIX 98 scheme). The shell also supports one by one file transfer.


```
    std::cout << "oohshell -s<d> *:PORT          : start server on all i-faces on PORT, -d, optional run as daemon. valid for -s only \n";
    std::cout << "oohshell -s<d> X.Y.Z.K:PORT <-d>  : start server on X.Y.Z.K interface, PORT, -d run as daemon. valid for -s only \n";
    std::cout << "oohshell -c X.Y.Z.K:PORT       : connects shell client to IP:PORT \n";
    std::cout << "oohshell -cp fname-remote@X.Y.Z.K:PORT  fname-local    : transfer from server locally \n";
    std::cout << "oohshell -cp fname-local fname-remote@X.Y.Z.K:PORT     : transfer file upload \n";
    std::cout << "oohshell stop                 : stops the server instance \n";
```

The oohshell was specifically developed for testing embedded linux devices where between each test the device had to be either rebooted and|or the software reinstalled on a clean slate.
The oohshell does not drop the client side prompt if the remote closes the connection.
To specifically exit from oohshell you have to close the remote shell (Ctrl+C), the from local off-line prompt to (Ctrl+Q). You can hold-on to the local client shell side opening and closing the remote shell by pressing Ctrl+C / Ctrl+O.  Alt+c switches from remote shell to local command oohhsell commands (in progress).
















`Vizit coinscode.com for mor info.`





_uen iu dont naeu uat iu duing pic: egeaile-divelopment_