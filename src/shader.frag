#version 330 core

in vec2 v_uv;

uniform sampler2D u_font; // the single-channel SDF atlas
uniform vec4 u_color;

out vec4 FragColor;

void main()
{
  // The atlas stores the signed distance in the red channel:
  // 0.5 is the glyph outline, below is outside, above is inside.
  float sdf = texture(u_font, v_uv).r;

  // fwidth() tells us how fast the SDF value changes across one
  // screen pixel. It adapts the antialiasing width to the on-screen
  // size of the SDF, so the same atlas stays smooth at any zoom.
  float width = fwidth(sdf);

  // Cover the pixel fully when the distance is more than half a
  // screen pixel inside the outline, and fade it out smoothly
  // across roughly one pixel of the edge.
  float alpha = smoothstep(0.5 - width, 0.5 + width, sdf);

  FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
