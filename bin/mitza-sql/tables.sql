/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - getic.net - N/A
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

create table que (
qid int(11) NOT NULL AUTO_INCREMENT,
url varchar(255),
catid int(11) DEFAULT '0',
ppid int(11),
level int(11),
sessid int(11),
reindex int(11),
depth int(11),
maxlnks int(11),
externals int(11),
nlinks int(11),
PRIMARY KEY (qid)
) ENGINE=MyISAM;

CREATE TABLE srv (
srvid int(11) NOT NULL AUTO_INCREMENT,
ip6 varchar(32) DEFAULT '',
ports varchar(256) DEFAULT '',
srvname varchar(64) DEFAULT '',
ip varchar(16) DEFAULT '',
PRIMARY KEY (srvid),
UNIQUE KEY ip_UNIQUE (ip)
) ENGINE=MyISAM;


CREATE TABLE domain (
siteid int(11) NOT NULL AUTO_INCREMENT,
sessid int(11) DEFAULT '0',
catid int(11) DEFAULT '0',
name varchar(255) DEFAULT '',
title varchar(255) DEFAULT '',
idat date DEFAULT NULL,
depth int(11) DEFAULT '2',
logo varchar(128) DEFAULT '',
extrn tinyint(1) DEFAULT '0',
nlinks int(11) DEFAULT '0',
loca varchar(255) DEFAULT '',
srvip varchar(16) DEFAULT '',
PRIMARY KEY (siteid),
UNIQUE KEY name_UNIQUE (name)
) ENGINE = MYISAM;


create table links (
linkid int auto_increment primary key not null,
siteid int(11),
sessid int(11),
catid int(11),
plinkid int(11),
url varchar(255) not null,
title varchar(200),
dsc varchar(255),
name varchar(64),
txt mediumtext,
idat date,
size int(2) default 0,
md5 varchar(128),
vis int default 0,
dpth int default 0,
nlinks int default 0,
blks int default 0,
ldat varchar(32) default "",
tid  bigint(20) default 0,
checkit  int default 0,
flags bigint(20) default 0,
priority int default 1,
key url (url),
key md5 (md5)
) ENGINE = MYISAM;

create table assets (
siteid int(11) default 0,
sessid int(11) default 0,
plinkid int(11) default 0,
href varchar(255) default ""
) ENGINE = MYISAM;

create table categories(
k_id integer not null auto_increment primary key,
cat varchar(64)
) ENGINE = MYISAM;


create table sess
(
id int(11)
) ENGINE = MYISAM;

create table rel (
siteid int default 0,
plinkid int default 0,
c_id int
) ENGINE = MYISAM;

create table dict	(
wordid int primary key not null auto_increment,
wrd varchar(32) not null,
ksum int(11) default 0,
unique kw (wrd),
key wrd (ksum,wrd(10))) ENGINE = MYISAM;

create table words (
linkid int not null,
wordid int not null,
weight int(3) default 1,
count  int(11) default 1,
siteid int(11) default 0,
ksum int(11) default 0,
lng int(1) default 0,
key linkid(ksum,wordid,linkid),
key kid(wordid)) ENGINE = MYISAM;




