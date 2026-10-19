#include "base.h"

int main () {
  Window window = {0};

  // Creating window
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

  // Loading font
  const char* font_path = "./font2.ttf";
  FileData font_file = {0};
  if (!read_file(&font_file, font_path)) {
    log_error("Failed to load font: %s\n", font_path);
    return -1;
  }
  log_sucess("loaded %s: %zu bytes\n", font_path, font_file.size);

  // Initializing stb font
  stbtt_fontinfo font;

  if (!stbtt_InitFont(
        &font,
        font_file.data,
        stbtt_GetFontOffsetForIndex(font_file.data, 0)) // Returns the index to the first font in the file
  ) {
    log_error("Failed to parse font file\n");
    return -1;
  }
  log_sucess("Font sucessfully parsed\n");

  // Adjust the font scaling according to pixel size
  f32 font_size = 64.0f;
  f32 scale = stbtt_ScaleForPixelHeight(&font, font_size);
  log_info("Font scale: %f\n", scale);

  // Calculate ascent, descent, line_gaps
  i32 ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
  log_info("Ascent: %d | Descent: %d | LineGap: %d\n", ascent, descent, line_gap);

  f32 ascent_px = ascent * scale;
  f32 descent_px = descent * scale;
  f32 line_gap_px = line_gap * scale;
  log_info("AscentPX: %.2f | DescentPX: %.2f | LineGapPX: %.2f\n", ascent_px, descent_px, line_gap_px);

  // Load SDF Glyph
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
    log_error("failed to generate SDF\n");
    return -1;
  }

  log_info("A SDF:\n");
  log_info("  width:  %d\n", width);
  log_info("  height: %d\n", height);
  log_info("  offset: %d, %d\n", x_offset, y_offset);

  stbtt_FreeSDF(sdf, NULL);

  while (!window.should_close) {
    imr_clear((v4) { 0.08f, 0.08f, 0.10f, 1.0f });

    window_update(&window);
  }

  window_delete(window);

  return 0;
}
