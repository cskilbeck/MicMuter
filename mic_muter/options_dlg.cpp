#include "framework.h"

namespace
{
    LOG_CONTEXT("options_dlg");

    using namespace chs;

    using mic_muter::settings;
    using mic_muter::settings_t;

    struct dlg_info_t
    {
        int page;
        HWND overlay_dlg;
    };

    settings_t old_settings;

    mic_muter::image overlay_img[mic_muter::num_overlay_ids];

    auto constexpr Ctl = GetDlgItem;

    HWND tooltip;
    TOOLINFO toolInfo = { 0 };

    //////////////////////////////////////////////////////////////////////

    dlg_info_t *dlg_info(HWND w)
    {
        return reinterpret_cast<dlg_info_t *>(GetWindowLongPtr(w, GWLP_USERDATA));
    }

    //////////////////////////////////////////////////////////////////////

    void enable_option_controls(HWND w, settings_t::overlay_setting const &s)
    {
        bool enable = s.enabled;

        EnableWindow(Ctl(w, IDC_COMBO_FADEOUT_AFTER), enable);

        enable &= s.fadeout_time != settings_t::fadeout_after::fadeout_never;

        EnableWindow(Ctl(w, IDC_COMBO_FADEOUT_SPEED), enable);
        EnableWindow(Ctl(w, IDC_SLIDER_FADE_TO), enable);

        InvalidateRect(Ctl(w, IDC_STATIC_OPTIONS_OVERLAY_IMAGE), nullptr, false);
    }

    //////////////////////////////////////////////////////////////////////

    void update_percent_str(HWND w, settings_t::overlay_setting const &s)
    {
        std::string percent_str{ "Hidden" };
        if(s.fadeout_to != 0) {
            percent_str = std::format("{}%", s.fadeout_to * 100 / 20);
        }
        toolInfo.lpszText = const_cast<char *>(percent_str.c_str());
        SendMessage(tooltip, TTM_UPDATETIPTEXT, 0, reinterpret_cast<WPARAM>(&toolInfo));
        LRESULT size = SendMessage(tooltip, TTM_GETBUBBLESIZE, 0, reinterpret_cast<WPARAM>(&toolInfo));
        int height = size >> 16;
        int width = size & 0xffff;

        HWND slider = Ctl(w, IDC_SLIDER_FADE_TO);

        RECT win_rect;
        GetWindowRect(slider, &win_rect);

        RECT track_rect;
        SendMessage(slider, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&track_rect));

        MapWindowPoints(slider, nullptr, reinterpret_cast<LPPOINT>(&track_rect), 2);

        uint32 y = track_rect.top - height * 140 / 100;
        uint32 x = track_rect.left + (track_rect.right - track_rect.left) / 2 - width / 2;
        SendMessage(tooltip, TTM_TRACKPOSITION, 0, (y << 16) | x);
    }

    //////////////////////////////////////////////////////////////////////

    void setup_option_controls(HWND w, settings_t::overlay_setting const &s)
    {
        Button_SetCheck(Ctl(w, IDC_CHECK_OVERLAY_ENABLE), s.enabled);
        ComboBox_SetCurSel(Ctl(w, IDC_COMBO_FADEOUT_AFTER), s.fadeout_time);
        ComboBox_SetCurSel(Ctl(w, IDC_COMBO_FADEOUT_SPEED), s.fadeout_speed);
        SendMessage(Ctl(w, IDC_SLIDER_FADE_TO), TBM_SETPOS, TRUE, s.fadeout_to);
        enable_option_controls(w, s);
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK overlay_dlg_proc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {

            // create tooltip for slider
            tooltip = CreateWindowEx(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0, dlg, nullptr,
                                     GetModuleHandle(nullptr), nullptr);

            toolInfo.cbSize = sizeof(toolInfo);
            toolInfo.hwnd = dlg;
            toolInfo.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
            toolInfo.uId = (UINT_PTR)Ctl(dlg, IDC_SLIDER_FADE_TO);
            toolInfo.lpszText = const_cast<char *>("HELLO");
            GetClientRect(dlg, &toolInfo.rect);
            SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

            // fill in main dialog info block

            HWND parent = GetParent(dlg);
            dlg_info_t *info = dlg_info(parent);
            info->overlay_dlg = dlg;

            // setup position within the tab control

            HWND tab_ctrl = Ctl(parent, IDC_OPTIONS_TAB_CTRL);
            RECT rc;
            GetWindowRect(tab_ctrl, &rc);
            MapWindowPoints(nullptr, parent, reinterpret_cast<LPPOINT>(&rc), 2);
            TabCtrl_AdjustRect(tab_ctrl, false, &rc);
            SetWindowPos(dlg, nullptr, rc.left, rc.top, 0, 0, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
            EnableThemeDialogTexture(dlg, ETDT_ENABLETAB);

            // setup the trackbar

            HWND trackbar = Ctl(dlg, IDC_SLIDER_FADE_TO);
            SendMessage(trackbar, TBM_SETRANGE, 0, (19 << 16) | 0);
            SendMessage(trackbar, TBM_SETTICFREQ, 1, 0);
            SendMessage(trackbar, TBM_SETLINESIZE, 0, 1);
            SendMessage(trackbar, TBM_SETPAGESIZE, 0, 5);

            // add strings to the combo boxes

            for(auto const *text : settings_t::fadeout_after_names) {
                ComboBox_AddString(Ctl(dlg, IDC_COMBO_FADEOUT_AFTER), text);
            }
            for(auto const *text : settings_t::fadeout_speed_names) {
                ComboBox_AddString(Ctl(dlg, IDC_COMBO_FADEOUT_SPEED), text);
            }

            // setup overlay image icon

            RECT img_rect;
            GetClientRect(Ctl(dlg, IDC_STATIC_OPTIONS_OVERLAY_IMAGE), &img_rect);
            int img_w = img_rect.right - img_rect.left;
            int img_h = img_rect.bottom - img_rect.top;

            for(int i = 0; i < mic_muter::num_overlay_ids; ++i) {
                auto id = static_cast<mic_muter::overlay_id>(i);
                overlay_img[i].create_from_svg(get_overlay_svg(id), img_w, img_h);
            }

            // set up the control values

            setup_option_controls(dlg, settings.overlay[info->page]);

            break;
        }

        case WM_DESTROY:
            for(auto &img : overlay_img) {
                img.destroy();
            }
            break;

        case WM_HSCROLL: {
            HWND slider = Ctl(dlg, IDC_SLIDER_FADE_TO);
            auto code = LOWORD(wParam);
            if(lParam == reinterpret_cast<LPARAM>(slider) && code != SB_ENDSCROLL && code != SB_THUMBPOSITION) {

                LRESULT pos = SendMessage(slider, TBM_GETPOS, 0, 0);
                dlg_info_t *info = dlg_info(GetParent(dlg));
                settings_t::overlay_setting &s = settings.overlay[info->page];

                s.fadeout_to = static_cast<byte>(pos);
                InvalidateRect(Ctl(dlg, IDC_STATIC_OPTIONS_OVERLAY_IMAGE), nullptr, false);

                SendMessage(tooltip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&toolInfo));
                update_percent_str(dlg, s);

                if(code != SB_THUMBTRACK) {
                    SetTimer(dlg, 100, 500, nullptr);
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = reinterpret_cast<LPNMHDR>(lParam);
            if(wParam == IDC_SLIDER_FADE_TO && nmhdr->code == NM_RELEASEDCAPTURE) {
                SetTimer(dlg, 100, 500, nullptr);
            }
            break;
        }

        case WM_TIMER:
            KillTimer(dlg, 100);
            SendMessage(tooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&toolInfo));
            break;

        case WM_DRAWITEM: {

            LPDRAWITEMSTRUCT d = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

            switch(d->CtlID) {

            case IDC_STATIC_OPTIONS_OVERLAY_IMAGE: {

                dlg_info_t *info = dlg_info(GetParent(dlg));
                settings_t::overlay_setting &s = settings.overlay[info->page];

                HDC dc = GetDC(dlg);
                HBRUSH fill_brush = CreateSolidBrush(GetBkColor(dc));
                ReleaseDC(dlg, dc);
                HGDIOBJ old_brush = SelectObject(d->hDC, fill_brush);
                FillRect(d->hDC, &d->rcItem, fill_brush);
                SelectObject(d->hDC, old_brush);
                DeleteObject(fill_brush);

                BLENDFUNCTION bf{};
                bf.BlendOp = AC_SRC_OVER;
                bf.SourceConstantAlpha = static_cast<BYTE>(s.fadeout_to * 255 / 20);
                bf.AlphaFormat = AC_SRC_ALPHA;

                if(s.fadeout_time == settings_t::fadeout_never) {
                    bf.SourceConstantAlpha = 255;
                }

                int w = overlay_img[info->page].width;
                int h = overlay_img[info->page].height;
                HDC src_dc = overlay_img[info->page].dc;
                AlphaBlend(d->hDC, 0, 0, w, h, src_dc, 0, 0, w, h, bf);
                break;
            }
            }
        }

        case WM_COMMAND: {
            HWND parent = GetParent(dlg);
            dlg_info_t *info = dlg_info(parent);
            settings_t::overlay_setting &s = settings.overlay[info->page];
            int action = HIWORD(wParam);
            switch(LOWORD(wParam)) {

            case IDC_CHECK_OVERLAY_ENABLE:
                s.enabled = Button_GetCheck(Ctl(dlg, IDC_CHECK_OVERLAY_ENABLE));
                enable_option_controls(dlg, s);
                break;

            case IDC_COMBO_FADEOUT_AFTER:
                if(action == CBN_SELCHANGE) {
                    s.fadeout_time = static_cast<byte>(ComboBox_GetCurSel(Ctl(dlg, IDC_COMBO_FADEOUT_AFTER)));
                    enable_option_controls(dlg, s);
                }
                break;

            case IDC_COMBO_FADEOUT_SPEED:
                if(action == CBN_SELCHANGE) {
                    s.fadeout_speed = static_cast<byte>(ComboBox_GetCurSel(Ctl(dlg, IDC_COMBO_FADEOUT_SPEED)));
                    enable_option_controls(dlg, s);
                }
                break;
            default:
                break;
            }
            break;
        }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////

    void update_hotkey_name(HWND dlg)
    {
        if(hotkey_scanning) {
            SetWindowText(Ctl(dlg, IDC_STATIC_HOTKEY_MESSAGE), "Press to set the hotkey...");
        } else {
            std::string hotkey_name = mic_muter::get_hotkey_name(hotkey_keycode, hotkey_modifiers);
            std::string hotkey_text = std::format("Hotkey is {}", hotkey_name);
            SetWindowText(Ctl(dlg, IDC_STATIC_HOTKEY_MESSAGE), hotkey_text.c_str());
        }
    }

    //////////////////////////////////////////////////////////////////////

    void setup_controls(HWND dlg)
    {
        Button_SetCheck(Ctl(dlg, IDC_CHECK_RUN_AT_STARTUP), settings.run_at_startup);
        int id = mic_muter::get_hotkey_index(hotkey_keycode);
        HWND hotkey_combo = Ctl(dlg, IDC_COMBO_HOTKEY);
        if(id == -1) {
            id = ComboBox_GetCount(hotkey_combo) - 1;
        }
        ComboBox_SetCurSel(hotkey_combo, id);
        Button_SetCheck(Ctl(dlg, IDC_CHECK_ALT), (hotkey_modifiers & keymod_alt) != 0);
        Button_SetCheck(Ctl(dlg, IDC_CHECK_CTRL), (hotkey_modifiers & keymod_ctrl) != 0);
        Button_SetCheck(Ctl(dlg, IDC_CHECK_SHIFT), (hotkey_modifiers & keymod_shift) != 0);
        Button_SetCheck(Ctl(dlg, IDC_CHECK_WINKEY), (hotkey_modifiers & keymod_winkey) != 0);
        update_hotkey_name(dlg);
    }

    //////////////////////////////////////////////////////////////////////

    void update_modifiers(BOOL state, int mask)
    {
        if(state) {
            hotkey_modifiers |= mask;
        } else {
            hotkey_modifiers &= ~mask;
        }
    }

    //////////////////////////////////////////////////////////////////////

    void enable_hotkey_controls(HWND dlg, bool enable)
    {
        int hide = enable ? SW_SHOW : SW_HIDE;
        ShowWindow(Ctl(dlg, IDC_CHECK_ALT), hide);
        ShowWindow(Ctl(dlg, IDC_CHECK_CTRL), hide);
        ShowWindow(Ctl(dlg, IDC_CHECK_SHIFT), hide);
        ShowWindow(Ctl(dlg, IDC_CHECK_WINKEY), hide);
        ShowWindow(Ctl(dlg, IDC_COMBO_HOTKEY), hide);
        SetWindowText(Ctl(dlg, IDC_BUTTON_CHOOSE_HOTKEY), enable ? "Scan" : "Cancel");
    }

    //////////////////////////////////////////////////////////////////////

    INT_PTR CALLBACK options_dlg_proc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch(message) {
        case WM_INITDIALOG: {

            // keyboard hook sends messages to this dialog, needs to know where it is
            options_dlg = dlg;

            // if dialog is cancelled, go back to these settings
            old_settings = settings;

            // init the tab control tabs' text
            HWND tab_ctrl = Ctl(dlg, IDC_OPTIONS_TAB_CTRL);
            TCITEM tie{};
            tie.mask = TCIF_TEXT;
            for(int i = 0; i < mic_muter::num_overlay_ids; ++i) {
                tie.pszText = const_cast<char *>(mic_muter::get_overlay_name(i));
                TabCtrl_InsertItem(tab_ctrl, i, &tie);
            }

            // create the overlay_settings child dialog
            HRSRC hrsrc = FindResource(NULL, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS_OVERLAY), RT_DIALOG);
            if(hrsrc == nullptr) {
                return false;
            }
            HGLOBAL hglb = LoadResource(GetModuleHandle(nullptr), hrsrc);
            if(hglb == nullptr) {
                return false;
            }
            DEFER(FreeResource(hglb));
            DLGTEMPLATE *dlg_template = reinterpret_cast<DLGTEMPLATE *>(LockResource(hglb));
            if(dlg_template == nullptr) {
                return false;
            }
            DEFER(UnlockResource(dlg_template));

            // attach some tracking info to the options dialog
            dlg_info_t *info = new dlg_info_t;
            info->page = static_cast<int>(lParam);
            SetWindowLongPtr(dlg, GWLP_USERDATA, (LONG_PTR)info);

            CreateDialogIndirect(GetModuleHandle(nullptr), dlg_template, dlg, overlay_dlg_proc);

            HWND combo = Ctl(dlg, IDC_COMBO_HOTKEY);
            for(size_t i = 0; i < mic_muter::num_hotkeys; ++i) {
                mic_muter::hotkey_t &hotkey = mic_muter::hotkeys[i];
                ComboBox_AddString(combo, hotkey.name);
            }
            ComboBox_AddString(combo, "Unknown Key");

            setup_controls(dlg);

            // show the requested overlay page
            TabCtrl_SetCurSel(tab_ctrl, lParam);

            break;
        }

        case WM_DESTROY: {
            delete dlg_info(dlg);
            PostMessage(overlay_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
            break;
        }

        case WM_ACTIVATE:
            if(wParam == WA_INACTIVE) {
                hotkey_scanning = false;
                enable_hotkey_controls(dlg, !hotkey_scanning);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDCANCEL:
                settings = old_settings;
                hotkey_keycode = settings.hotkey;
                hotkey_modifiers = settings.modifiers;
                EndDialog(dlg, IDCANCEL);
                break;
            case IDOK:
                settings.modifiers = hotkey_modifiers;
                settings.hotkey = hotkey_keycode;
                settings.save();
                EndDialog(dlg, IDCANCEL);
                break;
            case IDC_CHECK_RUN_AT_STARTUP:
                settings.run_at_startup = Button_GetCheck(Ctl(dlg, IDC_CHECK_RUN_AT_STARTUP)) != 0;
                break;
            case IDC_BUTTON_CHOOSE_HOTKEY:
                hotkey_scanning = !hotkey_scanning;
                update_hotkey_name(dlg);
                enable_hotkey_controls(dlg, !hotkey_scanning);
                break;
            case IDC_COMBO_HOTKEY: {
                if(HIWORD(wParam) == CBN_SELCHANGE) {
                    HWND combo = Ctl(dlg, IDC_COMBO_HOTKEY);
                    int chose = ComboBox_GetCurSel(combo);
                    if(chose != (ComboBox_GetCount(combo) - 1)) {
                        int id = std::clamp(chose, 0, static_cast<int>(mic_muter::num_hotkeys));
                        hotkey_keycode = mic_muter::hotkeys[id].key_code;
                        LOG_DEBUG("NEW CODE: ID: {}, CODE: {}", id, hotkey_keycode);
                        setup_controls(dlg);
                    }
                }
                break;
            }
            case IDC_CHECK_ALT:
                update_modifiers(Button_GetCheck(Ctl(dlg, IDC_CHECK_ALT)), keymod_alt);
                update_hotkey_name(dlg);
                break;
            case IDC_CHECK_CTRL:
                update_modifiers(Button_GetCheck(Ctl(dlg, IDC_CHECK_CTRL)), keymod_ctrl);
                update_hotkey_name(dlg);
                break;
            case IDC_CHECK_SHIFT:
                update_modifiers(Button_GetCheck(Ctl(dlg, IDC_CHECK_SHIFT)), keymod_shift);
                update_hotkey_name(dlg);
                break;
            case IDC_CHECK_WINKEY:
                update_modifiers(Button_GetCheck(Ctl(dlg, IDC_CHECK_WINKEY)), keymod_winkey);
                update_hotkey_name(dlg);
                break;
            }
            break;

        case WM_APP_HOTKEY_PRESSED:
            LOG_DEBUG("New hotkey: {:02x}, {:02x}", wParam, lParam);
            enable_hotkey_controls(dlg, true);
            setup_controls(dlg);
            break;

        case WM_NOTIFY: {
            LPNMHDR n = reinterpret_cast<LPNMHDR>(lParam);
            switch(n->idFrom) {
            case IDC_OPTIONS_TAB_CTRL:
                if(n->code == static_cast<UINT>(TCN_SELCHANGE)) {
                    HWND tab_ctrl = Ctl(dlg, IDC_OPTIONS_TAB_CTRL);
                    int page = TabCtrl_GetCurSel(tab_ctrl);
                    LOG_DEBUG("page: {}", page);
                    dlg_info_t *info = dlg_info(dlg);
                    info->page = std::clamp(page, 0, mic_muter::max_overlay_id);
                    setup_option_controls(info->overlay_dlg, settings.overlay[page]);
                }
                break;
            }
            break;
        }
        }
        return FALSE;
    }
}

namespace chs::mic_muter
{
    //////////////////////////////////////////////////////////////////////

    HRESULT show_options_dialog(bool muted, bool attached)
    {
        HWND w = FindWindow("#32770", "MicMuter options");
        if(w != nullptr) {
            BringWindowToTop(w);
            SetForegroundWindow(w);
            SetActiveWindow(w);
            SwitchToThisWindow(w, false);
            return S_OK;
        }
        LPSTR dlg_template = MAKEINTRESOURCE(IDD_DIALOG_OPTIONS);
        int page = get_overlay_id(muted, attached);
        DialogBoxParam(GetModuleHandle(nullptr), dlg_template, overlay_hwnd, options_dlg_proc, page);
        return S_OK;
    }
}
