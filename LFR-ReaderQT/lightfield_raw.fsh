#version 430
// opengl version 4.3

//#version 140
// opengl version 3.1

uniform sampler2D lightfield;
uniform mediump vec2 tex_dim;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 size_st;
uniform mediump vec3 white_balance;
uniform mediump vec2 centerLens_pos;
uniform mediump mat2 lenslet_m;
float focus_radius = 15;
uniform mediump float focus = 0;
uniform mediump mat3 ccm;
uniform mediump float modulationExposureBias;
uniform int view_mode = 3; // 0 - raw, 1 - bayer, 2 - demosaiced
uniform bool option_wb = true;
uniform bool option_ccm = true;
uniform bool option_gamma = true;
uniform int option_display_mode = 1; // 0 - uv, 1 - focus
int demosaicking_mode = 1; // 0 - nearest neighbour, 1 - bilinear
bool option_superresolution = true;
uniform bool is_raw = true;

varying mediump vec4 texc;
layout(location = 0) out vec3 color;

vec4 computeColorAt(vec2 texel_pos){
    // get position of current pixel
    texel_pos = texel_pos * tex_dim + vec2(0.5,0.5);
    ivec2 pix_pos = ivec2(texel_pos);

    // determine bayer position of pixel
    bvec2 bayer_pos = bvec2((pix_pos.x) % 2, (pix_pos.y+1) % 2);

    // accessing the center of the pixel
    vec2 texel_loc = (pix_pos + vec2(0.5f, 0.5f)) / tex_dim;

    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (bayer_pos == bvec2(0,0) ){ // green
        color.g = texture2D(lightfield, texel_loc).r;
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.r = texture2D(lightfield, vec2(1, 0) / tex_dim + texel_loc).r;
                color.b = texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r;
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.r = (texture2D(lightfield, vec2(1 ,0) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r ) / 2;
                color.b = (texture2D(lightfield, vec2(0,1)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r ) / 2;
            }
        }
    }
    else if (bayer_pos == bvec2(1,0) ){ // red
        color.r = texture2D(lightfield, texel_loc).r;
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.g = (texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r) / 2;
                color.b = texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r;
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.g = (texture2D(lightfield, vec2(0,1)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,0)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r ) / 4;
                color.b = (texture2D(lightfield, vec2(-1,-1)/ tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,1)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r) / 4;
            }
        }
    }
    else if (bayer_pos == bvec2(0,1) ){ // blue
        color.b = texture2D(lightfield, texel_loc).r;
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.g = (texture2D(lightfield, vec2(1,0) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(0,1) / tex_dim + texel_loc).r) / 2;
                color.r = texture2D(lightfield,  vec2(1,1) / tex_dim + texel_loc).r;
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.g = (texture2D(lightfield, vec2(0,1)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,0)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r ) / 4;
                color.r = (texture2D(lightfield, vec2(-1,-1)/ tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,1)  / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r) / 4;
            }
        }
    }
    else if (bayer_pos == bvec2(1,1) ){ // green
        color.g = texture2D(lightfield, texel_loc).r;
        if (view_mode >= 2){ // demosaicking
            if (demosaicking_mode == 0){ // nearest neighbour
                color.r = texture2D(lightfield, vec2(0,1)  / tex_dim + texel_loc).r;
                color.b = texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r;
            }
            else if(demosaicking_mode == 1){ // bilinear
                color.r = (texture2D(lightfield, vec2(0,1)   / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(0,-1)  / tex_dim + texel_loc).r ) / 2;
                color.b = (texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r
                         + texture2D(lightfield, vec2(1,0)  / tex_dim + texel_loc).r ) / 2;
            }
        }
    }

    if (option_wb){
        color.r *= white_balance.r;
        color.b *= white_balance.b;
    }

    if (option_ccm){
        //vec3 SatLvl = white_balance*ccm;
        //float min_sat = min(SatLvl.r, SatLvl.b);
        color.rgb = color.rgb*ccm;
        //color.r = max(0,min(color.r, min_sat)) / min_sat;
        //color.g = max(0,min(color.g, min_sat)) / min_sat;
        //color.b = max(0,min(color.b, min_sat)) / min_sat;
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
        color.r = pow(color.r, 0.496660f);
        color.g = pow(color.g, 0.496660f);
        color.b = pow(color.b, 0.496660f);

    }
    return color;
}

// get color from st uv coordinates
vec4 recalcPosAtSTUV(vec2 st, vec2 uv){
    vec4 fragcol=vec4(0,0,0,1);
    if(is_raw){
        if(int(st.y) % 2 == 1){
            vec2 texel_pos1 = (centerLens_pos + (lenslet_m * (st + vec2(0.5,0)) + uv)) / tex_dim;
            vec2 texel_pos2 = (centerLens_pos + (lenslet_m * (st - vec2(0.5,0)) + uv)) / tex_dim;
            fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2)) / 2;
        }
        else{
            vec2 pixel_pos = (lenslet_m * st);
            pixel_pos += uv;
            vec2 texel_pos = (centerLens_pos + pixel_pos) / tex_dim;
            fragcol = computeColorAt(texel_pos);
        }
    }
    else{
        if(int(st.y) % 2 == 1){
            vec2 texel_pos1 = (centerLens_pos + (lenslet_m * (st + vec2(0.5,0)) + uv)) / tex_dim;
            vec2 texel_pos2 = (centerLens_pos + (lenslet_m * (st - vec2(0.5,0)) + uv)) / tex_dim;
            fragcol = (texture2D(lightfield, texel_pos1) + texture2D(lightfield, texel_pos2)) / 2;
        }
        else{
            vec2 pixel_pos = (lenslet_m * st);
            pixel_pos += uv;
            vec2 texel_pos = (centerLens_pos + pixel_pos) / tex_dim;
            fragcol = texture2D(lightfield, texel_pos);
        }
    }
    return fragcol;
}


void main(void)
{
    //color = texture2D(lightfield,texc.st).rgb;
    //return;

    if (view_mode == 0)
        color = texture2D(lightfield,texc.st).rgb;
    else if(view_mode <= 2) // bayer & demosaic
        color = computeColorAt(texc.st).rgb;

    else if (view_mode > 2){

        // get st coordinate we are in
        vec2 st = vec2(1.0,1.0)-texc.st;

        // show all st-planes of all uvs
        vec2 uv_exact = texc.st * (lenslet_dim + vec2(1,1));
        vec2 uv = ivec2(uv_exact) - lenslet_dim/2; // from center
        st = vec2(1.0,1.0)-(uv_exact - ivec2(uv_exact)); // invert

        st = vec2(0.5,0.5) - st; // from center
        st *= vec2(floor(size_st.x)-2, size_st.y); // size of st plane
        //st *= size_st; // size of st plane
        st = floor(st + vec2(0.5f,0.5f)); // exact st position

        uv *= vec2(15,15) / lenslet_dim; //stretch uv coordinates from center
        color = recalcPosAtSTUV(st, uv).rgb;
    }
    /*
    else if(view_mode > 2){
        vec2 diff_from_raw_center = (centerLens_pos - (vec2(1.0f-texc.s, 1.0f-texc.t) * tex_dim));

        // converting to lenslet coordinate system
        //mat2 m = lenslet_m;
        //float a = m[0][0]; float b = m[1][0]; float c = m[0][1]; float d = m[1][1];
        //mat2 inverse_m = (1.0f/(a*d - b*c)) * mat2(vec2(d,-c),vec2(-b,a)); // column major
        vec2 currentLens_exact = inverse(lenslet_m) * diff_from_raw_center;

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
        // pixel from center to lenslet
        vec2 pixel_pos = lenslet_m * currentLens;

        // add pixel position in lens (to select different uv-image)
        // and add interleave position
        pixel_pos += lens_pos_view + ((option_superresolution) ? interleave_pos : ivec2(0,0));

        vec2 texel_pos = (centerLens_pos + pixel_pos) / tex_dim;

        // focus stuff
        int u = int(lens_pos_view.x);
        int v = int(lens_pos_view.y);

        //vec2 exact_lenslet_pos = lenslet_dim * focus * 0.001f;
        //vec2 lenslet_pos = vec2(int(exact_lenslet_pos.x), int(exact_lenslet_pos.y));
        //vec2 perc_lenslet_pos = exact_lenslet_pos-lenslet_pos;

        vec4 fragcol = vec4(0,0,0,0);
        for (int y=v-7; y <= v+7; y++){
            for (int x=u-7; x <= u+7; x++){
                vec2 x_y = vec2(x,y);
                if (length(x_y) < focus_radius){
                    x_y += -(x_y) * lenslet_dim * int(focus * 0.1f);
                    fragcol += interpolateST(texel_pos, x_y);
                }
            }
        }
        //gl_FragColor = fragcol / fragcol.a;
        gl_FragColor = computeColorAt(texel_pos);
    }*/

    // super resolution image by interleaving pixels
    // Means: Each point observed
    // t = 1/3 + n
    // n = 0,1,2,3,4,5...
    // a = db/(x + 1/3 + n)

    // d = lenslet_dim
    // ivec2 delta = ivec2(int(lenslet_dim.x +0.5f), int(lenslet_dim.y +0.5f));
    // b =? 3.699999999999999e-5;
    // a.x = d.x * b / (x + 1/3 + n)

    //gl_FragColor = vec4(1,1,0,1);

}
