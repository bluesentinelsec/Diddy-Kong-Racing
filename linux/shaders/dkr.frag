// Diddy Kong Racing host renderer - fragment shader.
// The "#version" line (and, for GL ES, the precision qualifiers) is prepended
// at runtime by linux/gfx.c.
//
// This is the fixed-function pipeline the port's first renderer used, written
// out: texture environment (modulate, or blend towards a constant), per-vertex
// fog factor, and the alpha test, in that order, exactly as GL 1.x applied them.

in vec2 vUV;
in float vFog;
in vec4 vColor;

uniform sampler2D uTex;
uniform int uTexMode;    // 0 = untextured, 1 = texel * colour, 2 = lerp colour -> uEnvColor by texel
uniform vec4 uEnvColor;
uniform int uFogEnable;
uniform vec4 uFogColor;
uniform float uAlphaRef; // fragments with alpha <= this are discarded (GL_GREATER)

out vec4 fragColor;

void main() {
    vec4 c = vColor;

    if (uTexMode == 1) {
        c = texture(uTex, vUV) * vColor;
    } else if (uTexMode == 2) {
        vec4 t = texture(uTex, vUV);
        c = vec4(mix(vColor.rgb, uEnvColor.rgb, t.rgb), vColor.a * t.a);
    }

    if (uFogEnable != 0) {
        c.rgb = mix(c.rgb, uFogColor.rgb, clamp(vFog, 0.0, 1.0));
    }

    if (c.a <= uAlphaRef) {
        discard;
    }

    fragColor = c;
}
