# gmd-shaders

This project makes it possible to use opengl shaders for Geometry Dash levels.

# Custom shaders

To use custom shaders you can use https://shadertoy.com to get shaders and https://shadertoyporter.github.io/ to port them and then paste them into the code.
You can only leave uniforms `resolution` and `time` but change:
* `uniform float time;` to `uniform float time = CC_Time.y;`
* `texture(...)` to `texture2D(...)`
* `iChannel0` to `sprite0`

Also it doesnt support additional textures, only 1(it is the level)

# How it works

Each frame it makes a "screenshot" of the level,ground,bg etc. and then applies a shader to it. Yes yes i know that it's extremly unefficient(40fps on 1080p) so it lowers the quality 1.5-2 times, it's not very noticable.

If you know a working different way to do it please text me in discord Jaan#2897

# Contacts

Jaan#2897 in discord; feel free to message me

# Credits

* matcool for menu-shaders
* gd programming for yes.
* gdl team for support
* kolyah35 for the original idea

# =)
