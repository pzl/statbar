Statbar
=======

`statbar` is a Linux status bar using [lemonbar](https://github.com/LemonBoy/bar) as a viewer, and optionally [conky](https://github.com/brndnmtthws/conky) for clickable popups. It allows for multiple visible bars, sharing the same data sources. I use this to put a status bar on multiple monitors, where each might have the CPU usage, without having to run `top` for each. Instead they share the output of one call to `top`.


Usage
-----

Call `statbar` to start a viewable statusbar. The data source daemon will start automatically. Call `statbar` again to start another viewable bar, and it will share the same data source, without starting any more.

Call `statd` if you want to start the daemon manually.




License
-------

Code licensed under the MIT license. See `LICENSE`.

Copyright 2015 Dan Panzarella