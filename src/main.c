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
  log_info("loaded %s: %zu bytes\n", font_path, font_file.size);

  while (!window.should_close) {
    imr_clear((v4) { 0.08f, 0.08f, 0.10f, 1.0f });

    window_update(&window);
  }

  window_delete(window);

  return 0;
}
