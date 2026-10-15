// Diddy Kong Racing host renderer - post-processing fragment shader.
// The "#version" line (and, for GL ES, the precision qualifiers) is prepended
// at runtime by linux/gfx.c.
//
// The scene is rendered into an offscreen framebuffer at the window's
// resolution and this pass copies it to the screen, applying whichever effects
// are switched on. Each effect is a uniform so it can be toggled at runtime
// (see gfx.c for the keys); everything off is a plain copy.

in vec2 vUV;

uniform sampler2D uScene;
uniform vec2 uSceneLines; // the N64 framebuffer size (320x240): the scanline pitch is one per line
uniform int uScanlines;   // CRT-style scanlines: one dark line per N64 scanline
uniform float uScanlineStrength;

out vec4 fragColor;

void main() {
    vec4 c = texture(uScene, vUV);

    if (uScanlines != 0) {
        // One cycle per N64 line: bright at the line's centre, darkest between
        // two lines. The lift keeps the overall image from dimming too much.
        float phase = fract(vUV.y * uSceneLines.y);
        float line = 0.5 + 0.5 * cos(phase * 6.28318530718);
        float shade = mix(1.0 - uScanlineStrength, 1.0, line);
        c.rgb *= shade * (1.0 + 0.35 * uScanlineStrength);
    }

    fragColor = vec4(min(c.rgb, 1.0), 1.0);
}
