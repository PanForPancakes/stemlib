# stemlib

> https://gitlab.com/UnicornFreedom/stem  
> Lightweight internet bridge.  
> Created with OpenComputers in mind and goes with Lua API library.

This is C++ library to connect to stem servers and send/receive data.

`stemlib::Client` class automatically manages its subscriptions and makes sure traffic is not eaten by some obscure channel that no `stemlib::Listener` listens to it and makes easy using multiple `stemlib::Listener`s with one client, which lowers traffic usage even lower if you have high data throughput application.

(Uses `Poco::Net`)
