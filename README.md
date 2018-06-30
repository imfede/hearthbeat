# Hearthbeat

A simple project aimed at building a heartbeat algorithm for your little home computing device (the target is a raspberry pi).

## What it does

Once executed it will fork and the main process will terminate, creating a daemon (unless the `--no-fork` option is given, in which case the main process will do all of the work). It will then bind itself on a port of your chosing (default to `0xBEA7` or 48807 for you, fellow human) listening both for TCP and UDP. Upon receiving a connection over TCP (or a datagram over UDP) it will answer with a "bip" and then close the connection (or just forget about it in the UDP case).
Note: in case of forking the program will try to write the pid of the child process in `/var/hearthbeat/pid` so that systemd can monitor it.

## What it's good for

Its purpose is to implement a safe and lightweight endpoint for services like [uptime robot](https://uptimerobot.com/).

## How to install

It comes shipping with a very simple configuration so you just have to do:
```
./configure
make
sudo make install
```
This will also attempt to install a systemd unit file, useful to enable the service so that you can forget about it and just let it do its job.

# Conclusion

Constructive criticism is always welcomed, as are PR, issues, and beers. And to you, fellow developer, sysadmin, or just enthusiast, keep up the good work and spread the love :)