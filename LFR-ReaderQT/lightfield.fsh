uniform sampler2D texture;
uniform mediump vec2 tex_dim;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 centerLens_pos;
uniform mediump vec2 lens_pos_view;
uniform mediump mat2 lenslet_m;
varying mediump vec4 texc;

void main(void)
{
    vec2 diff_from_raw_center = -(centerLens_pos - (texc.st * tex_dim));
    vec2 currentLens = inverse(lenslet_m) * diff_from_raw_center;
    currentLens = vec2(int(currentLens.x + 0.5), int(currentLens.y + 0.5));     // Now we have the correct lense
    vec2 pixel_in_lense = currentLens + lens_pos_view;
    if(int(currentLens.y) % 2 == 1)
       pixel_in_lense.x += 0.5;
    vec2 pixel_in_lense_pos = lenslet_m * pixel_in_lense;
    vec2 texel_pos = (centerLens_pos + pixel_in_lense_pos) / tex_dim;
    gl_FragColor = texture2D(texture, texel_pos);
};

