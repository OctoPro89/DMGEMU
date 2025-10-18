#version 330 core

uniform float screenHeight;
uniform float cornerRadius;
uniform vec4 color;
uniform vec2 rectPos;
uniform vec2 rectSize;

out vec4 FragColor;

float roundedBoxSDF(vec2 p, vec2 b, float r) {
    return length(max(abs(p) - b + r, 0.0)) - r;
}

void main() {
    vec2 fragPos = vec2(gl_FragCoord.x, screenHeight - gl_FragCoord.y);

    vec2 halfSize = rectSize * 0.5;
    vec2 center = rectPos + halfSize;            // Center of rectangle

    float edgeSoftness = 0.5;

    vec2 p = fragPos - center;

    float dist = roundedBoxSDF(p, halfSize, cornerRadius);
    float alpha = 1.0 - smoothstep(0.0, edgeSoftness, dist);

    FragColor = vec4(color.rgb, alpha * color.a);
}
