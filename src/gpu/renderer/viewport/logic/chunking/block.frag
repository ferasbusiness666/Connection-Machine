#version 450

layout(location = 0) in vec2 inTex;
layout(location = 1) flat in uint inTexLayer;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2DArray displayTexture; // changed

void main() {
    outColor = texture(displayTexture, vec3(inTex, inTexLayer));
}
