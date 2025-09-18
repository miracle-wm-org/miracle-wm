uniform sampler2D tex;
vec4 sample_to_rgba(in vec2 texcoord) {
     vec4 col = texture2D(tex, texcoord);
     float s = (col[0] + col[1] + col[2]) / 3.0;
     return vec4(s, s, s, col[3]);
}