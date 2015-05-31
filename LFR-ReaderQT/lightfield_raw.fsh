uniform sampler2D texture;
uniform mediump vec2 tex_dim;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 white_balance;
uniform mediump vec2 centerLens_pos;
uniform mediump vec2 lens_pos_view;
uniform mediump mat2 lenslet_m;
varying mediump vec4 texc;

vec4 computeColorAt(vec2 texel_pos){
    // get position of current pixel
    ivec2 pix_pos = ivec2(int(texel_pos.x * tex_dim.x), int(texel_pos.y * tex_dim.y));

    // determine bayer position of pixel
    int x = (pix_pos.x) % 2;
    int y = (pix_pos.y+1) % 2;

    vec2 pix_loc = pix_pos;

    // accessing the center of the pixel
    pix_loc += vec2(0.5f, 0.5f);

    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (x == 0 && y == 0 ){ // green
        //color.r = texture2D(texture, (pix_loc + vec2(1,0))  / tex_dim).r * white_balance.x;
        color.g = texture2D(texture,  pix_loc               / tex_dim).g;
        //color.b = texture2D(texture, (pix_loc + vec2(0,-1)) / tex_dim).b * white_balance.y;
    }
    else if (x == 1 && y == 0 ){ // red
        color.r = texture2D(texture,  pix_loc               / tex_dim).r * white_balance.x;
        //color.g = texture2D(texture, (pix_loc + vec2(-1,0)) / tex_dim).g;
        //color.b = texture2D(texture, (pix_loc + vec2(-1,-1))/ tex_dim).b * white_balance.y;
    }
    else if (x == 0 && y == 1 ){ // blue
        //color.r = texture2D(texture, (pix_loc + vec2(1,1))  / tex_dim).r * white_balance.x;
        //color.g = texture2D(texture, (pix_loc + vec2(0,1))  / tex_dim).g;
        color.b = texture2D(texture, (pix_loc)              / tex_dim).b * white_balance.y;
    }
    else if (x == 1 && y == 1 ){ // green
        //color.r = texture2D(texture, (pix_loc + vec2(0,1))  / tex_dim).r * white_balance.x;
        color.g = texture2D(texture,  pix_loc               / tex_dim).g;
        //color.b = texture2D(texture, (pix_loc + vec2(-1,0)) / tex_dim).b * white_balance.y;
    }

    return color;
}

void main(void)
{
    //gl_FragColor = texture2D(texture,texc.st);
    gl_FragColor = computeColorAt(texc.st);

    /*vec2 diff_from_raw_center = -(centerLens_pos - (texc.st * tex_dim));
    vec2 currentLens = inverse(lenslet_m) * diff_from_raw_center;
    currentLens = vec2(int(currentLens.x + 0.5), int(currentLens.y + 0.5));     // Now we have the correct lense
    vec2 pixel_in_lense = currentLens + lens_pos_view;
    if(int(currentLens.y) % 2 == 1)
       pixel_in_lense.x += 0.5;
    vec2 pixel_in_lense_pos = lenslet_m * pixel_in_lense;
    vec2 texel_pos = (centerLens_pos + pixel_in_lense_pos) / tex_dim;

    gl_FragColor = computeColorAt(texel_pos);*/


};

