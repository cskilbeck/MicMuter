#pragma once

namespace chs::mic_muter
{
    struct notification_icon
    {
        HRESULT load(HWND hwnd);
        HRESULT update(HWND hwnd, bool attached, bool muted);
        HRESULT destroy(HWND hwnd);
    };
}
