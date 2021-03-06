Statbar
=======

`statbar` is a Linux status bar using [lemonbar](https://github.com/LemonBoy/bar) as a viewer, and optionally [conky](https://github.com/brndnmtthws/conky) for clickable popups. It allows for multiple visible bars, sharing the same data sources. I use this to put a status bar on multiple monitors, where each might have the CPU usage, without having to run `top` for each. Instead they share the output of one call to `top`.


Requirements
------------
- **Linux**: There are a few Linux-specific commands. Could be modified to run on other platforms. Open an issue on github if you'd like support.
- **Lemonbar**: [github](https://github.com/LemonBoy/bar) or [XFT fork](https://github.com/krypt-n/bar).
- **Conky** _[optional]_: used as multi-line popup windows.

Fonts I'm using:

- Terminus (for text)
- [icons-in-terminal](https://github.com/pzl/icons-in-terminal) for icons

Fonts are specified in `src/client.c` when spawning lemonbar. Change to whatever font string you'd use.

Icons are specified in `src/common.c` in `set_environment` call. The icon strings are passed as environment variables to the modules and popups.


That's all that's needed for the status client itself. Individual modules may depend on certain commands being installed. But modules can be easily tailored to whatever your system uses. Or disabled (like, in the case of GPU if you have integrated graphics, or bluetooth if you don't have it).


Usage
-----

Call `statbar` to start a viewable statusbar. The data source daemon will start automatically. Call `statbar` again to start another viewable bar, and it will share the same data source, without starting any more. `statbar` may optionally take one argument, which is the size and position of the bar, specified like common window geometry in X11:  WIDTHxHEIGHT+X+Y. This defaults to `1920x22+0+0` when unspecified.

Call `statd` if you want to start the daemon manually.


Screenshots
-----------

default statbar:

![statbar](http://pzl.github.io/statbar/base.png)


with a few modules clicked on:

![clicks](http://pzl.github.io/statbar/clicks.png)


For fun, on my machine clicking the Arch logo at the end triggers a command adapted from [this reddit thread](https://www.reddit.com/r/unixporn/comments/35m141/i3lock_why_blur_the_beauty/) that locks the screen. Currently, this script isn't shipped with statbar

![locked](http://pzl.github.io/statbar/locked.png)



License
-------

Code licensed under the MIT license. See `LICENSE`.

Copyright 2015 Dan Panzarella
