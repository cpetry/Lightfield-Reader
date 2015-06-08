#version 140

uniform sampler2D renderedTexture;
//uniform sampler2D lightfield;
uniform mediump vec2 tex_dim;
uniform mediump vec2 uv_coord;
uniform mediump vec2 lenslet_dim;
uniform mediump vec2 size_st;
uniform mediump vec2 lens_pos_view;
uniform int view_mode = 3; // 0 - raw, 1 - bayer, 2 - demosaiced
float focus_radius = 7;
uniform float focus = 0;
varying mediump vec4 texc;

void main(void)
{

    if (view_mode == 4){
        ivec2 uv_size = ivec2(15,15);
        ivec2 uv = ivec2(int(lens_pos_view.x + lenslet_dim.x/2),int(lens_pos_view.y + lenslet_dim.y/2));
        vec4 fragcol = vec4(0,0,0,0); //texture2DArray(texture_array, vec3(texc.st, v * u_size + u));\n"
        for (int y=max(uv.y-7,1); y <= min(uv.y+7,13); y++){
           for (int x=max(uv.x-7,1); x <= min(uv.x+7,13); x++){
              if (length(vec2(uv.x-x,uv.y-y)) > focus_radius)
                  continue;
              vec2 pos = vec2(x, y);
              vec2 texel_pos = texc.st - focus * vec2(uv.x-x,uv.y-y) * 0.01f;
              pos = (texel_pos + pos) /(lenslet_dim + vec2(1,1));
              fragcol += texture2D(renderedTexture, pos);
           }
        }

        gl_FragColor = fragcol / fragcol.a;
    }
    else
        gl_FragColor = texture2D(renderedTexture, vec2(texc.st));
    //gl_FragColor = vec4(focus,1-focus,0,1);
}

