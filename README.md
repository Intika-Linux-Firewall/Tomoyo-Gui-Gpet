# GPET (Gui Policy Editor for TOMOYO Linux)

Mirror of https://osdn.net/projects/gpet/

**Usage**

```
$ gksudo gpet
```

or

```
$ sudo gpet [{policy_dir|remote_ip:remote_port}] [namespace]
```

Example
    
```
$ sudo sh -c 'echo /usr/sbin/gpet >> /etc/{ccs|tomoyo}/manager.conf'
$ sudo {ccs|tomoyo}-loadpolicy -m < /etc/{ccs|tomoyo}/manager.conf
```

**Install**

Build / Install / Uninstall

```
$ sudo apt-get install gcc make libncurses-dev
$ sudo apt-get install intltool libgtk2.0-dev libgconf2-dev
$ tar xjvf gpet-0.4.tar.bz2
$ cd gpet-0.4
$ ./configure --prefix /usr
$ make
$ sudo make install
```

**Uninstall**

```
$ sudo make uninstall
```

**Install Location**

```
/usr
 |-- sbin
 |   `-- gpet
 `-- share
     |-- doc
     |   `-- gpet
     |       |-- AUTHORS
     |       |-- COPYING
     |       |-- ChangeLog
     |       |-- ChangeLog.ja
     |       |-- INSTALL
     |       |-- NEWS
     |       |-- README
     |       |-- gpet.desktop
     |       `-- gpetrc.sample
     |-- gpet
     |   `-- pixmaps
     |       `-- tomoyo.png
     `-- locale
         `-- ja
             `-- LC_MESSAGES
                 `-- gpet.mo
```
