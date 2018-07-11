# Hearthbeat

A simple project aimed at building a heartbeat algorithm for your little home computing device (such as a raspberry pi).

It consist of two parts: `beat` and `monitor`

# Beat

This is basically a simple server which, upon receiving a TCP connection or UDP packet will send back a short answer and kill the connection (or just forget about it in the UDP case).

The listening port can be chosen using the `--port n` parameter and it will default to `0xBEA7` (or `48807` for you, fellow human).

Normally this executable will fork itself and let the parent die, thus becoming a daemon. This is a useful behaviour but it can be disabled using the `--no-fork` option. Once the program becomes a daemon it will write its PID in `/etc/hearthbeat/pid`.

# Monitor

This is the client for the aforementioned server. It will send short UDP messages to a target periodically, if it doesn't receive a message back after some time has passed it will send a "Host down" Telegram notification. Unsurprisingly, when it receives an answer it will send a "Host up" notification.

Its behaviour is controlled by config files. Please pay attention to this: the parser for this files is homemade and not really smart, so observe the following rules:

- every line must declare a property or be a comment or be empty
- a property declaration must follow the form `<key>=<value>`, there is no trimming anywhere, every space in the line will be considered intentional and part of the key/value
- a comment is a line whose first character is `#`
- the last line must be empty

## Files:

### `/etc/hearthbeat/telegram`
```conf
# userid is your account id on telegram
# it can be found using the get_my_id bot
user_id=<telegram_userid>

# token is the token of your bot. if you don't
# have one just message BotFather to create a new one
token=<telegram_token>
```

### `/etc/hearthbeat/monitor`
```conf
# the name of the monitor, used in the logs and in the notifications
myname=local

# targets in the form <name>:<host>:<port>
target=target1:localhost:48807
target=target2:127.0.0.2:12345

# UDP polling interval (in seconds)
poll_interval=1

# timeout (in seconds) after which a host is considered down
# every response from the target will reset this timer
err_interval=15
```

## How to install

First, make sure you have `libcurl`, `libevent`, and `argp` installed. If you don't know where to look to do that this tool probably isn't for you. 

Hearthbeat comes shipping with a very simple configuration so you just have to do:
```
./configure
make
sudo make install
```
This also attempt to install a systemd unit file, useful to enable the service so that you can forget about it and just let it do its job.

**Note**: this is how I made it work on my machines, you might want to inspect the contents of the files generated by the `configure` script (`Makefile`, `*.service`) before calling make to check if this approach will work for your case.

# Conclusion

Constructive criticism is always welcomed, as are PR, issues, and beers. And to you, fellow developer, sysadmin, or just enthusiast, keep up the good work and spread the love :)