#version 430
// opengl version 4.3

//#version 140
// opengl version 3.1

uniform sampler2D lightfield;
uniform mediump vec2 tex_dim;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 lenslet_coord;
uniform mediump vec2 size_st;
uniform mediump vec3 white_balance;
uniform mediump vec2 centerLens_pos;
uniform mediump mat2 lenslet_m;
uniform mediump mat2 R_m;
uniform highp mat4 H;
uniform highp mat4 H_inv;
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

vec4 recalcPosAtUVST_Cho(vec2 uv, vec2 st){
    vec4 fragcol=vec4(0,0,0,1);
    vec2 exact = vec2(0.5,0.5);
    //return computeColorAt(texc.st);
    vec2 img_pos = vec2(1.0,1.0) - texc.st; // invert
    img_pos *= size_st;
    //vec2 hex = /*size_st/2.0*/ - img_pos * 2; // from center + flip
    vec2 hex = ivec2(img_pos*2); // exact st position
    //hex += centerLens_pos * lenslet_dim;
    //hex = R_m * hex;
    //hex = lenslet_m * R_m * hex;

    if(is_raw){
        if(int(hex.y) % 4 == 0){
            //return vec4(0,0,0,1);
            if (int(hex.x) % 2 == 0){ // 1
                //return vec4(0,0,0,1);
                vec2 img_pos1 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 img_pos2 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 img_pos3 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 texel_pos1 = (centerLens_pos + img_pos1 + exact) / tex_dim;
                vec2 texel_pos2 = (centerLens_pos + img_pos2 + exact) / tex_dim;
                vec2 texel_pos3 = (centerLens_pos + img_pos3 + exact) / tex_dim;
                fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2) + computeColorAt(texel_pos3)) / 3;
            }
            else{                    // 2
                //return vec4(0,0,0,1);
                vec2 img_pos1 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 img_pos2 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 img_pos3 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 texel_pos1 = (centerLens_pos + img_pos1 + exact) / tex_dim;
                vec2 texel_pos2 = (centerLens_pos + img_pos2 + exact) / tex_dim;
                vec2 texel_pos3 = (centerLens_pos + img_pos3 + exact) / tex_dim;
                fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2) + computeColorAt(texel_pos3)) / 3;
            }
        }
        else if(int(hex.y) % 4 == 1){
            if (int(hex.x) % 2 == 0){// 3
                //return vec4(0,0,0,1);
                vec2 img_pos1 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 img_pos2 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 texel_pos1 = (centerLens_pos + img_pos1) / tex_dim;
                vec2 texel_pos2 = (centerLens_pos + img_pos2) / tex_dim;
                fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2)) / 2;
            }
            else{                    // 4
                //return vec4(0,0,0,1);
                img_pos = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos));
                vec2 texel_pos1 = ivec2(centerLens_pos + img_pos) / tex_dim;
                fragcol = (computeColorAt(texel_pos1));
            }
        }
        else if(int(hex.y) % 4 == 2){
            if (int(hex.x) % 2 == 0){
                //return vec4(0,0,0,1);
                vec2 img_pos1 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 img_pos2 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 img_pos3 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 texel_pos1 = (centerLens_pos + img_pos1) / tex_dim;
                vec2 texel_pos2 = (centerLens_pos + img_pos2) / tex_dim;
                vec2 texel_pos3 = (centerLens_pos + img_pos3) / tex_dim;
                fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2) + computeColorAt(texel_pos3)) / 3;
            }
            else{
                //return vec4(0,0,0,1);
                vec2 img_pos1 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 img_pos2 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 img_pos3 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 texel_pos1 = (centerLens_pos + img_pos1) / tex_dim;
                vec2 texel_pos2 = (centerLens_pos + img_pos2) / tex_dim;
                vec2 texel_pos3 = (centerLens_pos + img_pos3) / tex_dim;
                fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2) + computeColorAt(texel_pos3)) / 3;
            }
        }
        else if(int(hex.y) % 4 == 3){
            if (int(hex.x) % 2 == 0){
                //return vec4(0,0,0,1);
                img_pos = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 texel_pos1 = ivec2(centerLens_pos + img_pos) / tex_dim;
                fragcol = (computeColorAt(texel_pos1));
            }
            else{
                //return vec4(0,0,0,1);
                vec2 img_pos1 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 img_pos2 = lenslet_m * R_m * (size_st/2.0 - ivec2(img_pos) + vec2(0.5,0));
                vec2 texel_pos1 = (centerLens_pos + img_pos1) / tex_dim;
                vec2 texel_pos2 = (centerLens_pos + img_pos2) / tex_dim;
                fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2)) / 2;
            }
        }


    }
    return fragcol;
}

// get color from st uv coordinates
vec4 recalcPosAtUVST(vec2 uv, vec2 st){
    vec4 fragcol=vec4(0,0,0,1);
    if(is_raw){


        if(int(st.y) % 2 == 1){
            vec2 st_ = st - vec2(0.5,0);// + vec2(11,10);
            //st_ = lenslet_m/14.2857 * st_;
            //st_ = lenslet_m * st_;
            //st_ = R_m * st_;
            vec4 ijkl = vec4(H[1][0]*uv.x + H[2][0]*st_.x + H[3][0]*1,
                        H[1][1]*uv.y + H[2][1]*st_.y + H[3][1]*1,
                        H[1][2]*uv.x + H[2][2]*st_.x + H[3][2]*1,
                        H[1][3]*uv.y + H[2][3]*st_.y + H[3][3]*1);
            /*uvst = vec4(H[0][1]*uv.x + H[0][2]*st_.x + H[0][3]*1,
                             H[1][1]*uv.y + H[1][2]*st_.y + H[1][3]*1,
                             H[2][1]*uv.x + H[2][2]*st_.x + H[2][3]*1,
                             H[3][1]*uv.y + H[3][2]*st_.y + H[3][3]*1);*/
            ijkl.xy = lenslet_m * ijkl.xy;
            ijkl.xy = R_m * ijkl.xy;
            //vec2 texel_pos2 = (centerLens_pos + (ijkl.xy + ijkl.zw)) / tex_dim;
            vec2 texel_pos2 = (centerLens_pos + (lenslet_m*R_m * st_ + uv)) / tex_dim;
            //fragcol = (computeColorAt(texel_pos1) + computeColorAt(texel_pos2)) / 2;
            fragcol = (computeColorAt(texel_pos2));// + computeColorAt(texel_pos2)) / 2;
        }
        else{
            st = st;// + vec2(11,10);
            //st = lenslet_m/14.2857 * st;
            //st = lenslet_m * st;
            //st = R_m * st;
            vec4 ijkl = vec4(H[1][0]*uv.x + H[2][0]*st.x + H[3][0]*1,
                             H[1][1]*uv.y + H[2][1]*st.y + H[3][1]*1,
                             H[1][2]*uv.x + H[2][2]*st.x + H[3][2]*1,
                             H[1][3]*uv.y + H[2][3]*st.y + H[3][3]*1);
            /*vec4 uvst = vec4(H[0][1]*uv.x + H[0][2]*st.x + H[0][3]*1,
                             H[1][1]*uv.y + H[1][2]*st.y + H[1][3]*1,
                             H[2][1]*uv.x + H[2][2]*st.x + H[2][3]*1,
                             H[3][1]*uv.y + H[3][2]*st.y + H[3][3]*1);*/
            ijkl.xy = lenslet_m * ijkl.xy;
            ijkl.xy = R_m * ijkl.xy;
            //vec2 texel_pos = (centerLens_pos + (ijkl.xy + ijkl.zw)) / tex_dim;
            vec2 texel_pos = (centerLens_pos + (lenslet_m*R_m * st + uv)) / tex_dim;
            fragcol = computeColorAt(texel_pos);
        }
    }
    else{
        if(int(st.y) % 2 == 1){
            vec2 st_ = st + vec2(0.5,0);
            /*vec4 uvst = vec4(H[1][0]*kl.x + H[2][0]*ij_.x + H[3][0]*1,
                             H[1][1]*kl.y + H[2][1]*ij_.y + H[3][1]*1,
                             H[1][2]*kl.x + H[2][2]*ij_.x + H[3][2]*1,
                             H[1][3]*kl.y + H[2][3]*ij_.y + H[3][3]*1);
            uvst.st *= lenslet_m;
            //vec2 texel_pos1 = (centerLens_pos + (lenslet_m * (uvst.st) + uvst.wz)) / tex_dim;*/
            vec2 texel_pos1 = (centerLens_pos + (R_m*lenslet_m * st + uv)) / tex_dim;

            st_ = st - vec2(0.5,0);
            /*uvst = vec4(H[1][0]*kl.x + H[2][0]*ij_.x + H[3][0]*1,
                        H[1][1]*kl.y + H[2][1]*ij_.y + H[3][1]*1,
                        H[1][2]*kl.x + H[2][2]*ij_.x + H[3][2]*1,
                        H[1][3]*kl.y + H[2][3]*ij_.y + H[3][3]*1);
            uvst.st *= uvst.st;
            //vec2 texel_pos2 = (centerLens_pos + (uvst.st + uvst.wz)) / tex_dim;*/
            vec2 texel_pos2 = (centerLens_pos + (R_m*lenslet_m * st + uv)) / tex_dim;
            fragcol = (texture2D(lightfield, texel_pos1) + texture2D(lightfield, texel_pos2)) / 2;
        }
        else{
            /*vec4 uvst = vec4(H[1][0]*kl.x + H[2][0]*ij.x + H[3][0]*1,
                             H[1][1]*kl.y + H[2][1]*ij.y + H[3][1]*1,
                             H[1][2]*kl.x + H[2][2]*ij.x + H[3][2]*1,
                             H[1][3]*kl.y + H[2][3]*ij.y + H[3][3]*1);
            uvst.st = lenslet_m * uvst.st;
            //vec2 texel_pos = (centerLens_pos + (uvst.st + uvst.wz)) / tex_dim;*/
            vec2 texel_pos = (centerLens_pos + (R_m*lenslet_m * st + uv)) / tex_dim;
            fragcol = texture2D(lightfield, texel_pos);
        }
    }
    return fragcol;
}

vec4 recalcPosAtIJKL(vec2 ij, vec2 kl){
    vec4 fragcol=vec4(0,0,0,1);
    if(is_raw){
        if(int(kl.y) % 2 == 1){
            vec2 texel_pos1 = (centerLens_pos + (lenslet_m*R_m * (kl + vec2(0.5,0)) + ij)) / tex_dim;
            //vec2 texel_pos2 = (centerLens_pos + (R_m*lenslet_m*lenslet_dim * (kl - vec2(0.5,0)) + ij)) / tex_dim;
            fragcol = (computeColorAt(texel_pos1));// + computeColorAt(texel_pos2)) / 2;
        }
        else{
            vec2 texel_pos = (centerLens_pos + (lenslet_m*R_m * kl + ij)) / tex_dim;
            fragcol = computeColorAt(texel_pos);
        }
    }
    else{
        if(int(kl.y) % 2 == 1){
            vec2 texel_pos1 = (centerLens_pos + (lenslet_m*R_m * (kl + vec2(0.5,0)) + ij)) / tex_dim;
            vec2 texel_pos2 = (centerLens_pos + (lenslet_m*R_m * (kl - vec2(0.5,0)) + ij)) / tex_dim;
            fragcol = (texture2D(lightfield, texel_pos1) + texture2D(lightfield, texel_pos2)) / 2;
        }
        else{
            vec2 texel_pos = (centerLens_pos + (lenslet_m*R_m * kl + ij)) / tex_dim;
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

        // show all st-planes of all uv's
        vec2 uv_exact = texc.st * vec2(15.0,15.0);//(lenslet_dim + vec2(1,1)); //
        vec2 uv = vec2(uv_exact) - vec2(15.0,15.0)/2.0;//lenslet_dim/2.0; // from center

        vec2 st = vec2(1.0,1.0) - (uv_exact - ivec2(uv_exact)); // invert
        st *= size_st; // size of st plane
        st = size_st/2.0 - st; // from center + flip
        st = floor(st + vec2(0.5f,0.5f)); // exact st position

        uv *= lenslet_dim/vec2(15.0,15.0); //stretch uv coordinates from center

        //color = recalcPosAtUVST(uv, st);
        //color = recalcPosAtUVST_Cho(uv, st);
        color = recalcPosAtIJKL(uv, st);
    }
}
