#pragma once

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter::svg
{
    extern char const *microphone_mute_svg;
    extern char const *microphone_normal_svg;
    extern char const *microphone_disconnected_svg;

    extern char const *microphone_mute_small_svg;
    extern char const *microphone_normal_small_svg;
    extern char const *microphone_disconnected_small_svg;
}

namespace chs::mic_muter
{
    char const *get_overlay_svg(overlay_id id);
    char const *get_small_overlay_svg(overlay_id id);

    struct image
    {
        LOG_CONTEXT("image");

        HDC dc{ nullptr };
        HGDIOBJ old_bmp{ nullptr };
        HBITMAP bmp{ nullptr };
        int width{ 0 };
        int height{ 0 };

        //////////////////////////////////////////////////////////////////////

        HRESULT create_from_svg(char const *svg, int w, int h);
        void destroy();
    };
}