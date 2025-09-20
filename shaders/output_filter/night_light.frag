uniform sampler2D tex;
vec4 sample_to_rgba(in vec2 texcoord) {
    vec4 color = texture2D(tex, texcoord);
    float temperature = 0.5f;
    temperature = clamp(temperature, 0.5, 1.5);

    float r = color.r;
    float g = color.g;
    float b = color.b;

    r *= 1.0;
    g *= mix(1.0, 0.9, 1.0 - temperature);  // Slightly reduce green
    b *= mix(1.0, 0.6, 1.0 - temperature);  // Heavily reduce blue

    return vec4(r, g, b, color.a);
}

