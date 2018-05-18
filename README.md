# waymonad-clients
Client applications designed for waymonad. **Should** work on other compositors, but no guarantees given.

# What's this?

This is intended as collection of clients that are written with
[waymonad](https://github.com/waymonad/waymonad) in
mind.
Similar to the clients that [sway](https://github.com/swaywm/sway)/i3 etc. have.

Most clients provided here should be runable on other
[wlroots](https://github.com/swaywm/wlroots) clients, and `waymonad` should be
capable of running most clients from other compositors, this isn't a walled
garden.

But there may be some clients (`swaymsg` is an example on the sway end) that won't work,
they should be marked.


## Why not just contribute to sway clients?

The sway clients are a bit more minimal but stable than these will most likely
be.

The clients in here should fit the general lean aspect of `waymonad`, but
are allowed to be a bit more "playful".

E.g. the background client here does some image processing, and picks "fitting"
images on its own while swaybg takes a single file and sets it up as
background.

## On archlinux:
`PKG_CONFIG_PATH=/usr/lib/imagemagick6/pkgconfig meson build` to use the
imagemagick version 6, instead of 7
