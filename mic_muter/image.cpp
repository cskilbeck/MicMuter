//////////////////////////////////////////////////////////////////////

#include "framework.h"

//////////////////////////////////////////////////////////////////////

namespace
{

#include "images/microphone_mute_svg.h"
#include "images/microphone_normal_svg.h"
#include "images/microphone_base_svg.h"

    char const *overlay_svg[chs::mic_muter::num_overlay_ids] = { microphone_mute_svg, microphone_normal_svg,
                                                                 microphone_base_svg };
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

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
}