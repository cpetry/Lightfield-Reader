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
uniform int demosaicking_mode = 1; // 0 - nearest neighbour, 1 - bilinear, 2 -
bool option_superresolution = true;
uniform bool is_raw = true;
float PI_3  = 3.141592653589f / 3.0f;

varying mediump vec4 texc;
layout(location = 0) out vec4 color;

vec4 computeColorAt(vec2 texel_pos){
    // get position of current pixel
    texel_pos = texel_pos * tex_dim;// + vec2(0.5,0.5);
    ivec2 pix_pos = ivec2(texel_pos);

    // determine bayer position of pixel
    bvec2 bayer_pos = bvec2((pix_pos.x) % 2, (pix_pos.y+1) % 2);

    // accessing the center of the pixel
    vec2 texel_loc = (pix_pos + vec2(0.5f, 0.5f)) / tex_dim;

    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (bayer_pos == bvec2(0,0) ){ // green Gr
                                   //       bg
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
            else if(demosaicking_mode == 2){ // high quality
                // R at green in R row, B column
                color.r = (texture2D(lightfield, texel_loc).r * 5
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,0) / tex_dim + texel_loc).r * 4
                           + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r * 4) / 8;
                // B at green in R row, B column
                color.b = (texture2D(lightfield, texel_loc).r * 5
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(0,1) / tex_dim + texel_loc).r * 4
                           + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r * 4) / 8;
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
            else if(demosaicking_mode == 2){ // high quality
                // G at R locations
                color.g = (texture2D(lightfield, texel_loc).r * 4
                           + texture2D(lightfield, vec2(0,1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(1,0) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * -1) / 8;
                // B at R locations
                color.b = (texture2D(lightfield, texel_loc).r * 6
                           + texture2D(lightfield, vec2(1,1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * -1.5
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * -1.5
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * -1.5
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * -1.5) / 8;
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
            else if(demosaicking_mode == 2){ // high quality
                // G at B locations
                color.g = (texture2D(lightfield, texel_loc).r * 4
                           + texture2D(lightfield, vec2(0,1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(1,0) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * -1) / 8;
                // R at B locations
                color.r = (texture2D(lightfield, texel_loc).r * 6
                           + texture2D(lightfield, vec2(1,1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r * 2
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * -1.5
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * -1.5
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * -1.5
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * -1.5) / 8;
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
            else if(demosaicking_mode == 2){ // high quality
                // R at green in B row, R column
                color.r = (texture2D(lightfield, texel_loc).r * 5
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(0,1) / tex_dim + texel_loc).r * 4
                           + texture2D(lightfield, vec2(0,-1) / tex_dim + texel_loc).r * 4) / 8;
                // B at green in B row, R column
                color.b = (texture2D(lightfield, texel_loc).r * 5
                           + texture2D(lightfield, vec2(0,2) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(0,-2) / tex_dim + texel_loc).r * 0.5
                           + texture2D(lightfield, vec2(1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-1,-1) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(2,0) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(-2,0) / tex_dim + texel_loc).r * -1
                           + texture2D(lightfield, vec2(1,0) / tex_dim + texel_loc).r * 4
                           + texture2D(lightfield, vec2(-1,0) / tex_dim + texel_loc).r * 4) / 8;
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
        color.r = pow(color.r, 0.4166600108f); // 4166600108
        color.g = pow(color.g, 0.4166600108f);
        color.b = pow(color.b, 0.4166600108f);

    }
    return color;
}

// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
vec4 colorFromBarycentricCoords(vec2 p, vec2 a, vec2  b, vec2 c){
        vec2 v0 = b - a, v1 = c - a, v2 = p - a;
        float d00 = dot(v0, v0);
        float d01 = dot(v0, v1);
        float d11 = dot(v1, v1);
        float d20 = dot(v2, v0);
        float d21 = dot(v2, v1);
        float denom = d00 * d11 - d01 * d01;
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;
        //return computeColorAt(a) * u + computeColorAt(b) * v + computeColorAt(c) * w;
        return vec4(1,0,0,1) * u + vec4(0,0,1,1) * v + vec4(0,1,0,1) * w;
        if (u > v && u > w)
            return vec4(1,0,0,1);
        else if (v > u && v > w)
            return vec4(0,0,1,1);
        else if (w > u && w > v)
            return vec4(0,1,0,1);
        else
            return vec4(0,0,0,1);
}

// get color from st uv coordinates
vec4 recalcPosAtSTUV(vec2 st, vec2 uv){
    vec4 fragcol=vec4(0,0,0,1);
    if(is_raw){
        /* barycentric coordinates
        float sty_perc = abs(st.y - int(st.y));
        float stx_perc = abs(st.x - int(st.x));
        //if (int(st.y) % 2 == 1)
        //    stx_perc += 0.5;

        if (stx_perc > 0.5){
            stx_perc -= 0.5;
            sty_perc = 1-sty_perc;
        }
        if (int(st.y) % 2 == 1){
            stx_perc = abs(stx_perc-0.5);
            sty_perc = 1-sty_perc;
        }
        vec2 pixel_pos = (lenslet_m * st);

        if (st.x > 0 && st.y > 0){
            if(atan(sty_perc, stx_perc) < PI_3){
                vec2 a = (centerLens_pos + (lenslet_m * (vec2(int(st.x),int(st.y)+1) + vec2(0.5,0)) + uv)) / tex_dim;
                vec2 b = (centerLens_pos + (lenslet_m * (vec2(int(st.x),int(st.y))) + uv)) / tex_dim;
                vec2 c = (centerLens_pos + (lenslet_m * (vec2(int(st.x)+1,int(st.y))) + uv)) / tex_dim;
                vec2 p = (centerLens_pos + pixel_pos) / tex_dim;
                fragcol = colorFromBarycentricCoords(p, a, b, c);
            }
            else{
                vec2 a = (centerLens_pos + (lenslet_m * (vec2(int(st.x),int(st.y)+1) + vec2(0.5,0)) + uv)) / tex_dim;
                vec2 b = (centerLens_pos + (lenslet_m * (vec2(int(st.x),int(st.y)+1) - vec2(0.5,0)) + uv)) / tex_dim;
                vec2 c = (centerLens_pos + (lenslet_m * (vec2(int(st.x),int(st.y))) + uv)) / tex_dim;
                vec2 p = (centerLens_pos + pixel_pos) / tex_dim;
                fragcol = colorFromBarycentricCoords(p, a, b, c);
            }
        }
        else
            fragcol = vec4(0,0,0,1);*/

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
        color = texture2D(lightfield,texc.st);
    else if(view_mode <= 2) // bayer & demosaic
        color = computeColorAt(texc.st);

    else if (view_mode > 2){

        // get st coordinate we are in
        vec2 st = vec2(1.0,1.0)-texc.st;

        // show all st-planes of all uvs
        vec2 uv_exact = texc.st * (lenslet_dim + vec2(1,1));
        vec2 uv = ivec2(uv_exact) - lenslet_dim/2; // from center
        st = vec2(1.0,1.0)-(uv_exact - ivec2(uv_exact)); // invert

        st = vec2(0.5,0.5) - st; // from center + flip
        st *= vec2(int(size_st.x), size_st.y); // size of st plane
        //st *= size_st; // size of st plane
        st = floor(st + vec2(0.5f,0.5f)); // exact st position

        uv *= vec2(15,15) / lenslet_dim; //stretch uv coordinates from center
        color = recalcPosAtSTUV(st, uv); // st -312.5 - 312.5, -217.5 - 217.5
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
