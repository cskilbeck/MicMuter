//////////////////////////////////////////////////////////////////////

#include "framework.h"

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter::svg
{
#include "images/header/microphone_mute_svg.h"
#include "images/header/microphone_normal_svg.h"
#include "images/header/microphone_disconnected_svg.h"

#include "images/header/microphone_mute_small_svg.h"
#include "images/header/microphone_normal_small_svg.h"
#include "images/header/microphone_disconnected_small_svg.h"
}

//////////////////////////////////////////////////////////////////////

namespace
{
    char const *overlay_svg[chs::mic_muter::num_overlay_ids] = { chs::mic_muter::svg::microphone_mute_svg,
                                                                 chs::mic_muter::svg::microphone_normal_svg,
                                                                 chs::mic_muter::svg::microphone_disconnected_svg };

    char const *overlay_svg_small[chs::mic_muter::num_overlay_ids] = {
        chs::mic_muter::svg::microphone_mute_small_svg, chs::mic_muter::svg::microphone_normal_small_svg,
        chs::mic_muter::svg::microphone_disconnected_small_svg
    };
}

//////////////////////////////////////////////////////////////////////

namespace chs::mic_muter
{
    HRESULT image::create_from_svg(char const *svg, int w, int h)
    {
        destroy();

        HR(util::svg_to_bitmap(svg, w, h, &bmp));
        width = w;
        height = h;
        dc = CreateCompatibleDC(nullptr);
        old_bmp = SelectObject(dc, bmp);
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    void image::destroy()
    {
        if(old_bmp != nullptr && dc != nullptr) {
            SelectObject(dc, old_bmp);
            old_bmp = nullptr;
        }
        if(bmp != nullptr) {
            DeleteObject(bmp);
            bmp = nullptr;
        }
        if(dc != nullptr) {
            DeleteDC(dc);
            dc = nullptr;
        }
    }

    //////////////////////////////////////////////////////////////////////

    char const *get_overlay_svg(overlay_id id)
    {
        int index = std::clamp(static_cast<int>(id), 0, max_overlay_id);
        return overlay_svg[index];
    }

    //////////////////////////////////////////////////////////////////////

    char const *get_small_overlay_svg(overlay_id id)
    {
        int index = std::clamp(static_cast<int>(id), 0, max_overlay_id);
        return overlay_svg_small[index];
    }
}