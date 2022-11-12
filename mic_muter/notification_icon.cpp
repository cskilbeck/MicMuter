//////////////////////////////////////////////////////////////////////

#include "framework.h"

namespace
{
    //////////////////////////////////////////////////////////////////////

    LOG_CONTEXT("icon");

#if defined(_WIN64)

#if defined(_DEBUG)
    class __declspec(uuid("CC2EC94D-B8E2-4FF5-9C4C-41DCD4AC8D87")) icon_guid;
#else
    class __declspec(uuid("8312E0D8-5CB0-4799-B13F-B52B8AE6F61E")) icon_guid;
#endif

#else

#if defined(_DEBUG)
    class __declspec(uuid("428CA1C5-4EDD-4584-BCB4-559ACA5EF861")) icon_guid;
#else
    class __declspec(uuid("2B08A2EA-33B9-4DDD-A163-8208799ED9C4")) icon_guid;
#endif

#endif

    HICON icon[chs::mic_muter::num_overlay_ids];
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::load(HWND hwnd)
    {
        int w = GetSystemMetrics(SM_CXSMICON);
        int h = GetSystemMetrics(SM_CYSMICON);

        HR(chs::util::svg_to_icon(svg::microphone_mute_svg, w, h, &icon[overlay_id_muted]));
        HR(chs::util::svg_to_icon(svg::microphone_normal_svg, w, h, &icon[overlay_id_unmuted]));
        HR(chs::util::svg_to_icon(svg::microphone_disconnected_svg, w, h, &icon[overlay_id_disconnected]));

        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = hwnd;
        nid.uFlags = NIF_GUID | NIF_MESSAGE;
        nid.guidItem = __uuidof(icon_guid);
        nid.uCallbackMessage = WM_APP_NOTIFICATION_ICON;
        if(!Shell_NotifyIcon(NIM_ADD, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_ADD)");
        }

        nid.uVersion = NOTIFYICON_VERSION_4;
        if(!Shell_NotifyIcon(NIM_SETVERSION, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_SETVERSION)");
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::update(HWND hwnd, bool attached, bool muted)
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = hwnd;
        nid.hIcon = icon[get_overlay_id(muted, attached)];
        nid.uFlags = NIF_ICON | NIF_GUID;
        nid.guidItem = __uuidof(icon_guid);
        if(!Shell_NotifyIcon(NIM_MODIFY, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_MODIFY)");
        }
        return S_OK;
    }

    //////////////////////////////////////////////////////////////////////

    HRESULT notification_icon::destroy(HWND hwnd)
    {
        NOTIFYICONDATA nid = { sizeof(nid) };
        nid.hWnd = hwnd;
        nid.uFlags = NIF_GUID | NIF_MESSAGE;
        nid.uCallbackMessage = WM_APP_NOTIFICATION_ICON;
        nid.guidItem = __uuidof(icon_guid);
        if(!Shell_NotifyIcon(NIM_DELETE, &nid)) {
            return WIN32_LAST_ERROR("Shell_NotifyIcon(NIM_DELETE)");
        }
        for(auto &ico : icon) {
            DestroyIcon(ico);
        }
        return S_OK;
    }
}
