Battstat is a simple battery monitor that I wrote for use with [dwm][1] and
[dunst][2], though it should be easy enough to use it with other window managers
and notification daemons. It reports the status of the battery continuously,
and provides low battery notifications via the notification daemon.

# Building

    make

# Installing

    cp battstat $somewhere_in_your_path

# Usage

When run, Battstat repeatedly outputs the status of the battery. It will look
something like:

    Charging (51%)

Or

    Discharging (32%), 1:05:23

A new status line should come in roughly once per second. In my case, I have
the following in my .xinitrc:

    battstat | while read battinfo; do
	    xsetroot -name "$(show-weather.sh) | $battinfo | $(date +'%a %d %h, %I:%M %p')"
    done &

Which puts the battery status, along with some other information, in the
dwm status bar (which just displays the root window's name). If you're using
another window manager, you will have to find your own way to display the
status.

Battstat will also emit two warnings to the running notification daemon (if
any), When the battery has less than 20 minutes remaining, and again when it
has less than 5 minutes remaining. It's worth noting that Battstat does not
link against libnotify, rather it invokes the notify-send command to send
messages instead (The libnotify api is much more complex to use than the cli).

# License

ISC, see COPYING.

[1]: http://dwm.suckless.org
[2]: http://www.knopwob.org/dunst
