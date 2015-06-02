#version 140

uniform sampler2D lightfield;
uniform mediump vec2 tex_dim;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 centerLens_pos;
uniform mediump vec2 lens_pos_view;
uniform mediump mat2 lenslet_m;
varying mediump vec4 texc;

void main(void)
{
    vec2 diff_from_raw_center = -(centerLens_pos - (texc.st * tex_dim));
    float a = lenslet_m[0][0];
    float b = lenslet_m[0][1];
    float c = lenslet_m[1][0];
    float d = lenslet_m[1][1];
    mediump mat2 inverse_m = (1.0f/(a*d - b*c)) * mediump mat2(vec2(d,-c),vec2(-b,a)); // column major
    vec2 currentLens = inverse_m * diff_from_raw_center;
    currentLens = vec2(int(currentLens.x + 0.5), int(currentLens.y + 0.5));     // Now we have the correct lense
    vec2 lens_coord = currentLens + lens_pos_view;
    if(int(currentLens.y) % 2 == 1)
       lens_coord.x += 0.5;
    vec2 texel_pos2 = (centerLens_pos + (lenslet_m * lens_coord)) / tex_dim;
    if(int(currentLens.y) % 2 == 1)
       lens_coord.x -= 1;
    vec2 texel_pos = (centerLens_pos + (lenslet_m * lens_coord)) / tex_dim;


    if(int(currentLens.y) % 2 == 1){
        gl_FragColor = (texture2D(lightfield, texel_pos) + texture2D(lightfield, texel_pos2)) / 2.0f;
    }
    else
        gl_FragColor = texture2D(lightfield, texel_pos);
    //gl_FragColor = texture2D(texture, texc.st);
}

