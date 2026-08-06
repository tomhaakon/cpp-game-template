#if defined(_WIN32) && TEYA_ENABLE_EDITOR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <dwmapi.h>
#include <windows.h>

extern "C" void *GetWindowHandle(void);

void applyEditorTitleBarTheme() {
    const auto window = static_cast<HWND>(GetWindowHandle());
    if (!window)
        return;

    const BOOL darkMode = TRUE;
    const COLORREF captionColor = RGB(24, 26, 31);
    const COLORREF textColor = RGB(238, 240, 245);
    // Unsupported attributes are safely ignored on older Windows versions.
    (void)DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode,
                                sizeof(darkMode));
    (void)DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &captionColor,
                                sizeof(captionColor));
    (void)DwmSetWindowAttribute(window, DWMWA_TEXT_COLOR, &textColor, sizeof(textColor));
}
#endif
