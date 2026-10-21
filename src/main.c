#include "base.h"

#define FONT_SIZE 300.0f
#define POS_X 200.0f
#define POS_Y 200.0f

typedef struct {
  f32 u0, v0;
  f32 u1, v1;

  f32 width, height;
  f32 bearing_x, bearing_y;

  f32 advance;
} SDFGlyph;

int main () {
  Window window = {0};

  if (!window_new(
    &window,
    "SDF Text",
    1000,
    700,
    (WindowSettings) {
      .fullscreen = false,
      .vsync = true,
    }
  )) {
    return 1;
  }

  // Load font
  const char* font_path = "./font2.ttf";
  FileData font_file = {0};

  if (!read_file(&font_file, font_path)) {
    log_error("Failed to load font: %s\n", font_path);
    return -1;
  }

  log_sucess("Loaded %s: %zu bytes\n", font_path, font_file.size);

  // Initialize stb_truetype
  stbtt_fontinfo font;

  if (!stbtt_InitFont(
    &font,
    font_file.data,
    stbtt_GetFontOffsetForIndex(font_file.data, 0)
  )) {
    log_error("Failed to parse font file\n");
    return -1;
  }

  log_sucess("Font successfully parsed\n");

  // Font size
  f32 scale = stbtt_ScaleForPixelHeight(&font, FONT_SIZE);

  // Font metrics
  i32 ascent;
  i32 descent;
  i32 line_gap;

  stbtt_GetFontVMetrics(
    &font,
    &ascent,
    &descent,
    &line_gap
  );

  f32 ascent_px = ascent * scale;
  f32 descent_px = descent * scale;
  f32 line_gap_px = line_gap * scale;

  log_info(
    "Ascent: %.2f | Descent: %.2f | LineGap: %.2f\n",
    ascent_px,
    descent_px,
    line_gap_px
  );

  // Create atlas
  #define ATLAS_SIZE 1024

  u8* atlas = mem_alloc(
    ATLAS_SIZE * ATLAS_SIZE
  );

  // Generate SDF for A
  SDFGlyph glyph = {0};

  i32 pen_x = 1;
  i32 pen_y = 1;

  i32 width;
  i32 height;
  i32 x_offset;
  i32 y_offset;

  u8* sdf = stbtt_GetCodepointSDF(
    &font,
    scale,
    'A',
    8,
    128,
    128.0f / 8.0f,
    &width,
    &height,
    &x_offset,
    &y_offset
  );

  if (!sdf) {
    log_error("Failed to generate SDF\n");
    return -1;
  }

  // Copy SDF into atlas
  for (i32 y = 0; y < height; y++) {
    memcpy(
      atlas + (pen_y + y) * ATLAS_SIZE + pen_x,
      sdf + y * width,
      width
    );
  }

  // Store atlas coordinates
  glyph.u0 = (f32) pen_x / ATLAS_SIZE;
  glyph.v0 = (f32) pen_y / ATLAS_SIZE;
  glyph.u1 = (f32) (pen_x + width) / ATLAS_SIZE;
  glyph.v1 = (f32) (pen_y + height) / ATLAS_SIZE;

  glyph.width = width;
  glyph.height = height;

  // Get horizontal metrics
  i32 advance_width;
  i32 left_bearing;

  stbtt_GetCodepointHMetrics(
    &font,
    'A',
    &advance_width,
    &left_bearing
  );

  glyph.advance = advance_width * scale;
  glyph.bearing_x = x_offset;
  glyph.bearing_y = -y_offset;

  // Position glyph on screen
  f32 x = POS_X;
  f32 y = POS_Y;

  f32 x0 = x + glyph.bearing_x;
  f32 y0 = y + glyph.bearing_y;

  f32 x1 = x0 + glyph.width;
  f32 y1 = y0 + glyph.height;

  // Position + UV
  f32 vertices[] = {
    x0, y0, glyph.u0, glyph.v0,
    x1, y0, glyph.u1, glyph.v0,
    x1, y1, glyph.u1, glyph.v1,

    x0, y0, glyph.u0, glyph.v0,
    x1, y1, glyph.u1, glyph.v1,
    x0, y1, glyph.u0, glyph.v1,
  };

  stbtt_FreeSDF(sdf, NULL);

  // ------------------------------------------------------------
  // OpenGL texture
  // ------------------------------------------------------------

  GLuint texture;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_MIN_FILTER,
    GL_LINEAR
  );

  glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_MAG_FILTER,
    GL_LINEAR
  );

  glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_WRAP_S,
    GL_CLAMP_TO_EDGE
  );

  glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_WRAP_T,
    GL_CLAMP_TO_EDGE
  );

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_R8,
    ATLAS_SIZE,
    ATLAS_SIZE,
    0,
    GL_RED,
    GL_UNSIGNED_BYTE,
    atlas
  );

  glBindTexture(GL_TEXTURE_2D, 0);

  mem_free(atlas);

  // ------------------------------------------------------------
  // VAO + VBO
  // ------------------------------------------------------------

  GLuint vao;
  GLuint vbo;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) 0);
  glEnableVertexAttribArray(0);

  // UV
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*) (2 * sizeof(f32)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // ------------------------------------------------------------
  // Shaders
  // ------------------------------------------------------------

  Shader shader;
  if (!shader_new_from_file(&shader, "./src/shader.vert", "./src/shader.frag")) {
    log_error("failed to load shaders\n");
    return -1;
  }

  // ------------------------------------------------------------
  // Rendering
  // ------------------------------------------------------------

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while (!window.should_close) {
    imr_clear((v4) {
      0.08f,
      0.08f,
      0.10f,
      1.0f
    });


    glUseProgram(shader);
    m4 projection = ortho_projection(0, window.width, 0, window.height, -1, 1);
    GLint projection_location = glGetUniformLocation(shader, "u_projection");
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection.m[0][0]);

    GLint color_location =
      glGetUniformLocation(shader, "u_color");

    glUniform4f(
      color_location,
      1.0f,
      1.0f,
      1.0f,
      1.0f
    );

    GLint font_location =
      glGetUniformLocation(shader, "u_font");

    glUniform1i(font_location, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(vao);

    glDrawArrays(
      GL_TRIANGLES,
      0,
      6
    );

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    window_update(&window);
  }

  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteTextures(1, &texture);

  window_delete(window);

  return 0;
}
