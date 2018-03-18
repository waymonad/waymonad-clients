# waymonad-clients background application

This client will set up a background surface for every output available on
`waymonad`.

It **currently** uses a custom `waymonad` specific protocol, this is only a
stop-gap measure until `surface-layers` is avaiable.
After it is ported to that, it should work on every supporting compositor.

### Usage:
	`background [directory]` (directory defaults to "." if omitted).

### Behaviour:
  * Searches directory (recursive) for image files (formats `imagemagick processes`)
  * Filters images by a (currently the only) fitting metric.
    * After ratio preserving scaling a maximum of 10% of the screen are free
  * Scales the image exact, and blures it
  * Scales the image ratio preserving and composits it over the blurred image
