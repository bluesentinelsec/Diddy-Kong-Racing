// Diddy Kong Racing host renderer - vertex shader.
// The "#version" line (and, for GL ES, the precision qualifiers) is prepended
// at runtime by linux/gfx.c, so this source serves both GL 3.3 core and
// OpenGL ES 3.0.
//
// Vertices arrive from the F3DDKR interpreter already projected to N64 screen
// space (320x240, origin top-left, y down) and perspective-divided, with their w
// restored by multiplying it back in: aPos = (x*w, y*w, z*w, w). Mapping that
// through an orthographic projection reproduces the exact screen position, while
// w on the wire makes the rasterizer interpolate UVs perspective-correctly.

in vec4 aPos;
in vec2 aUV;
in float aFog;
in vec4 aColor;

uniform vec2 uScreen; // N64 framebuffer size the interpreter draws in

out vec2 vUV;
out float vFog;
out vec4 vColor;

void main() {
    vUV = aUV;
    vFog = aFog;
    vColor = aColor;
    // glOrtho(0, W, H, 0, -1, 1): x -> 2x/W - 1, y -> 1 - 2y/H, z -> -z.
    gl_Position = vec4(aPos.x * (2.0 / uScreen.x) - aPos.w, aPos.w - aPos.y * (2.0 / uScreen.y), -aPos.z, aPos.w);
}
