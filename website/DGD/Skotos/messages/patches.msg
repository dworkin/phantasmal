From: "Christopher Allen"
To: DGD Mailing List
Subject: RE: [DGD]Patches
Date: Tue, 11 Apr 2000 15:27:07 -0700

The line of demarcation is becoming more specific as we do some architectural
partitioning out of SkotOS -- we have a number of layers above the kernel
library:

 -------------------------------------------------------------
/                          Game                               \
\                          World                              /
 -------------------------------------------------------------
/                          Game                               \
\                     Specific Mechanics                      /
 =============================================================
 -------------------------------------   ---------------------
/                WorldLib             \ /       IFLib         \
|   inventory            stance pose   |                      |
|  proximity  details  faces  desc     |   socials     verbs  |
\    exits  gender  volition  physics / \   adverbs  errors   /
 --- - - - - - - - - - - - - - - - --- \ \ -- - - - - - - - --
/                 DevLib                \ /   ToolLib         \
|     Services             Interpreters  |                    |
|                                        |    sam  |   dtd    |
|  sid  | devd  ||  state | forms | sami |  xmlgen | xmlparse |
\  logd | helpd ||        |       |     / \                   /
 -------------------------------------------------------------
/                                                             \
|                               System                        |
|                               Library                       |
|        Object Services          ||      Net Services        |
|                                 ||                          |
|  initd | idd | progdbd |syslogd || nptd | httpd | devuserd@ |
\        |     |         |        ||      |       |           /
 -------------------------------------------------------------
/                                                             \
|                               Kernel                        |
|                               Library                       |
|                                                             |
|    security  ownership  resources  thread-local  logon      |
\                                       storage               /
 -------------------------------------------------------------
/                                                             \
|                                D G D 				|
|                                                             |
|  LPC Compiler  |  Run-Time  |  Database  |  I/O  |  Parser  |
\                |            |            |       |          /
 -------------------------------------------------------------

It is the game world and game specific mechanics (such as the specifics of
combat) that is the property of the developer using our libraries.


------------------------------------------------------------------------
. Christopher Allen                                 Skotos Tech Inc. ..
.                           1512 Walnut St., Berkeley, CA 94709-1513 ..
. <http://www.Skotos.net>               o510/649-4030  f510/649-4034 ..
