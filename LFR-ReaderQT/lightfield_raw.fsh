#version 140

// opengl version 3.1

uniform sampler2D lightfield;
uniform mediump vec2 tex_dim;
uniform mediump vec2 lenslet_dim;
uniform mediump vec3 white_balance;
uniform mediump vec2 centerLens_pos;
uniform mediump vec2 lens_pos_view;
uniform mediump mat2 lenslet_m;
uniform mediump mat3 ccm;
uniform mediump float modulationExposureBias;
uniform int view_mode = 3; // 0 - raw, 1 - bayer, 2 - demosaiced
uniform bool option_wb = true;
uniform bool option_ccm = true;
uniform bool option_gamma = true;
uniform int demosaicking_mode = 1; // 0 - nearest neighbour, 1 - bilinear
uniform bool option_superresolution = true;
varying mediump vec4 texc;

vec4 computeColorAt(vec2 texel_pos){
    // get position of current pixel
    ivec2 pix_pos = ivec2(int(texel_pos.x * tex_dim.x + 0.5f), int(texel_pos.y * tex_dim.y + 0.5f));

    // determine bayer position of pixel
    int x = (pix_pos.x)   % 2;
    int y = (pix_pos.y+1) % 2;

    vec2 pix_loc = pix_pos;

    // accessing the center of the pixel
    pix_loc += vec2(0.5f, 0.5f);

    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (x == 0 && y == 0 ){ // green
        color.g = texture2D(lightfield, pix_loc / tex_dim).r;
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.r = texture2D(lightfield, (pix_loc + vec2(1,0))  / tex_dim).r * ((option_wb) ? white_balance.r : 1.0f);
                color.b = texture2D(lightfield, (pix_loc + vec2(0,-1)) / tex_dim).r * ((option_wb) ? white_balance.b : 1.0f);
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.r = (texture2D(lightfield, (pix_loc + vec2(1,0))   / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(-1,0))  / tex_dim).r ) / 2
                        * ((option_wb) ? white_balance.r : 1.0f);
                color.b = (texture2D(lightfield, (pix_loc + vec2(0,1))   / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(0,-1))  / tex_dim).r ) / 2
                        * ((option_wb) ? white_balance.b : 1.0f);
            }
        }
    }
    else if (x == 1 && y == 0 ){ // red
        color.r = texture2D(lightfield, pix_loc / tex_dim).r * ((option_wb) ? white_balance.r : 1.0f);
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.g = (texture2D(lightfield, (pix_loc + vec2(-1,0)) / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(0,-1)) / tex_dim).r) / 2;
                color.b = texture2D(lightfield, (pix_loc + vec2(-1,-1))/ tex_dim).r
                        * ((option_wb) ? white_balance.b : 1.0f);
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.g = (texture2D(lightfield, (pix_loc + vec2(0,1))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(0,-1)) / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,0))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(-1,0))  / tex_dim).r ) / 4;
                color.b = (texture2D(lightfield, (pix_loc + vec2(-1,-1)) / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(-1,1))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,1))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,-1))  / tex_dim).r) / 4
                        * ((option_wb) ? white_balance.b : 1.0f);
            }
        }
    }
    else if (x == 0 && y == 1 ){ // blue
        color.b = texture2D(lightfield, pix_loc / tex_dim).r * ((option_wb) ? white_balance.b : 1.0f);
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.g = (texture2D(lightfield, (pix_loc + vec2(1,0)) / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(0,1)) / tex_dim).r) / 2;
                color.r = texture2D(lightfield, (pix_loc + vec2(1,1))  / tex_dim).r
                        * ((option_wb) ? white_balance.r : 1.0f);
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.g = (texture2D(lightfield, (pix_loc + vec2(0,1))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(0,-1)) / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,0))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(-1,0))  / tex_dim).r ) / 4;
                color.r = (texture2D(lightfield, (pix_loc + vec2(-1,-1)) / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(-1,1))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,1))  / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,-1))  / tex_dim).r) / 4
                        * ((option_wb) ? white_balance.r : 1.0f);
            }
        }
    }
    else if (x == 1 && y == 1 ){ // green
        color.g = texture2D(lightfield, pix_loc / tex_dim).r;
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.r = texture2D(lightfield, (pix_loc + vec2(0,1))  / tex_dim).r * ((option_wb) ? white_balance.r : 1.0f);
                color.b = texture2D(lightfield, (pix_loc + vec2(-1,0)) / tex_dim).r * ((option_wb) ? white_balance.b : 1.0f);
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.r = (texture2D(lightfield, (pix_loc + vec2(0,1))   / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(0,-1))  / tex_dim).r ) / 2
                        * ((option_wb) ? white_balance.r : 1.0f);
                color.b = (texture2D(lightfield, (pix_loc + vec2(-1,0))   / tex_dim).r
                         + texture2D(lightfield, (pix_loc + vec2(1,0))  / tex_dim).r ) / 2
                        * ((option_wb) ? white_balance.b : 1.0f);
            }
        }
    }

    if (option_ccm){
        vec3 SatLvl = white_balance*ccm;
        float min_sat = min(SatLvl.r, SatLvl.b);
        color.rgb = color.rgb*ccm;
        color.r = max(0,min(color.r, min_sat)) / min_sat;
        color.g = max(0,min(color.g, min_sat)) / min_sat;
        color.b = max(0,min(color.b, min_sat)) / min_sat;
        //color.r = min_sat;
        //color.g = min_sat;
        //color.b = min_sat;
        //color.rgb = vec3(min_sat, min_sat, min_sat);
    }
    // gamma value can not be achieved directly... so guessing with exposure bias
    // gamma-> c = c^g
    // exposure-> c = c*e
    // gamma_guessed = sqrt(e)
    if(option_gamma){
        //color.r = pow(color.r, sqrt(modulationExposureBias));
        //color.g = pow(color.g, sqrt(modulationExposureBias));
        //color.b = pow(color.b, sqrt(modulationExposureBias));
        color.r = pow(color.r, 0.416660f);
        color.g = pow(color.g, 0.416660f);
        color.b = pow(color.b, 0.416660f);

    }
    return color;
}

void main(void)
{
    if (view_mode == 0)
        gl_FragColor = texture2D(lightfield,texc.st);
    else if(view_mode <= 2) // bayer & demosaic
        gl_FragColor = computeColorAt(texc.st);

    else if(view_mode > 2){
        vec2 diff_from_raw_center = (centerLens_pos - (vec2(1.0f-texc.s, 1.0f-texc.t) * tex_dim));

        // converting to lenslet coordinate system
        mat2 m = lenslet_m;
        float a = m[0][0]; float b = m[1][0]; float c = m[0][1]; float d = m[1][1];
        mat2 inverse_m = (1.0f/(a*d - b*c)) * mat2(vec2(d,-c),vec2(-b,a)); // column major
        vec2 currentLens_exact = inverse_m * diff_from_raw_center;

        vec2 currentLens = vec2(int(currentLens_exact.x + 0.5), int(currentLens_exact.y + 0.5));
        // Now we have the correct lense

        // split 1 pixel into 9 Pixel
        ivec2 interleave_pos = ivec2(int((int(currentLens.x) - currentLens_exact.x-0.5f)/0.33333),
                                     int((int(currentLens.y) - currentLens_exact.y-0.5f)/0.33333))
                                + ivec2((currentLens.x < 0) ? -1 : 1,
                                        (currentLens.y < 0) ? -1 : 1);
        interleave_pos = -interleave_pos;

        if(int(currentLens.y) % 2 == 1)
           currentLens.x += 0.5;

        // convert back to pixel coordinate system
        vec2 pixel_pos = lenslet_m * currentLens;

        // add pixel position in lens (to select different uv-image)
        // and add interleave position
        pixel_pos += lens_pos_view + ((option_superresolution) ? interleave_pos : ivec2(0,0));

        vec2 texel_pos = (centerLens_pos + pixel_pos) / tex_dim;

        gl_FragColor = computeColorAt(texel_pos);
    }

    // super resolution image by interleaving pixels
    // Means: Each point observed
    // t = 1/3 + n
    // n = 0,1,2,3,4,5...
    // a = db/(x + 1/3 + n)

    // d = lenslet_dim
    // ivec2 delta = ivec2(int(lenslet_dim.x +0.5f), int(lenslet_dim.y +0.5f));
    // b =? 3.699999999999999e-5;
    // a.x = d.x * b / (x + 1/3 + n)

    gl_FragColor = vec4(1,1,0,1);

}
