// Diddy Kong Racing host renderer - post-processing vertex shader.
// The "#version" line (and, for GL ES, the precision qualifiers) is prepended
// at runtime by linux/gfx.c.
//
// One quad covering the 4:3 image area of the window, in normalised device
// coordinates, with texture coordinates over the scene framebuffer.

in vec2 aPos;
in vec2 aUV;

out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
