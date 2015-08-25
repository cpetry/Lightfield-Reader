#version 140

uniform sampler2D renderedTexture;
//uniform sampler2D lightfield;
uniform mediump vec2 tex_dim;
uniform mediump vec2 uv_coord;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 size_st;
uniform mediump vec2 lens_pos_view;
uniform int view_mode = 4; // 0 - raw, 1 - bayer, 2 - demosaiced, 3 - uv, 4 - focus
uniform int decode_mode = 0; // 0 - meta, 1 - dan, 2 - cho
uniform float focus_spread = 1;
float focus_radius = 7;
mat3 gauss_sharp = mat3(0,-1,0,-1,5,-1,0,-1,0);
uniform float focus = 0;
varying mediump vec4 texc;

void main(void)
{
    // Cho does not yet have a uvst representation. therefor just show the view created in lightfield_raw
    if (decode_mode == 2){
        gl_FragColor = texture2D(renderedTexture, vec2(texc.st));
        return;
    }

    if (view_mode == 4){
        ivec2 uv_size = ivec2(15,15);
        ivec2 uv = ivec2(int(lens_pos_view.x),int(lens_pos_view.y));
        vec4 fragcol = vec4(0,0,0,0); //texture2DArray(texture_array, vec3(texc.st, v * u_size + u));\n"

        /*for (int y=-1; y <= +1; y++){
           for (int x=-1; x <= +1; x++){
               vec2 pos = (texc.st + uv + vec2(x, y) + lenslet_dim/2) /(lenslet_dim + vec2(1,1));
               fragcol += texture2D(renderedTexture, pos) * gauss_sharp[x+1][y+1];
           }
        }*/

        for (int y=max(uv.y-7,-7); y <= min(uv.y+7,7); y++){
           for (int x=max(uv.x-7,-7); x <= min(uv.x+7,7); x++){
              float dist = length(vec2(x,y));
              if (dist > focus_radius)
                  continue;
              vec2 pos = vec2(x, y)+uv;
              //vec2 scale = (vec2(0.5,0.5) - texc.st) * dist * focus_spread + vec2(0.5,0.5);
              vec2 texel_pos = texc.st - focus * 0.001f * vec2(x,y);
              pos = (texel_pos + pos + lenslet_dim/2) /(lenslet_dim + vec2(1,1));
              fragcol += texture2D(renderedTexture, pos) * max(pow(dist,1),1);
           }
        }

        gl_FragColor = fragcol / fragcol.a;
    }
    else
        gl_FragColor = texture2D(renderedTexture, vec2(texc.st));
}

