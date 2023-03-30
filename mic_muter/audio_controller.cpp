#include "framework.h"

namespace
{
    LOG_CONTEXT("audio");
}

namespace chs::mic_muter
{
    // ----------------------------------------------------------------------
    //  Call when the app is done with this object before calling release.
    //  This detaches from the endpoint and releases all audio service references.
    // ----------------------------------------------------------------------

    void audio_controller::close()
    {
        detach_from_endpoint();

        if(endpoint_registered) {
            enumerator->UnregisterEndpointNotificationCallback(this);
            endpoint_registered = false;
        }
        enumerator.Reset();
        audio_endpoint.Reset();
        volume_control.Reset();
    }

    // ----------------------------------------------------------------------
    //  Initialize this object.  Call after constructor.
    // ----------------------------------------------------------------------

    HRESULT audio_controller::init()
    {
        HR(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                            reinterpret_cast<LPVOID *>(enumerator.GetAddressOf())));
        HR(enumerator->RegisterEndpointNotificationCallback(this));
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  get current state of mic - plugged in/muted
    // ----------------------------------------------------------------------

    HRESULT audio_controller::get_mic_info(bool *present, bool *muted)
    {
        if(present == nullptr || muted == nullptr) {
            return ERROR_BAD_ARGUMENTS;
        }

        std::lock_guard lock(endpoint_mutex);

        *present = volume_registered;
        if(!volume_registered) {
            return S_OK;
        }
        BOOL is_muted;
        HR(volume_control->GetMute(&is_muted));
        *muted = static_cast<bool>(is_muted);
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Toggle mute status of microphone
    // ----------------------------------------------------------------------

    HRESULT audio_controller::toggle_mute()
    {
        std::lock_guard lock(endpoint_mutex);

        if(volume_control == nullptr) {
            return E_FAIL;
        }

        LOG_INFO("Toggling mute...");

        BOOL current_mute_state;
        HR(volume_control->GetMute(&current_mute_state));
        HR(volume_control->SetMute(!current_mute_state, nullptr));
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Start monitoring the current default input audio device
    // ----------------------------------------------------------------------

    wchar_t const *formfactor_names[] = { L"RemoteNetworkDevice",
                                          L"Speakers",
                                          L"LineLevel",
                                          L"Headphones",
                                          L"Microphone",
                                          L"Headset",
                                          L"Handset",
                                          L"UnknownDigitalPassthrough",
                                          L"SPDIF",
                                          L"DigitalAudioDisplayDevice",
                                          L"UnknownFormFactor" };

    struct state_string
    {
        DWORD state;
        wchar_t const *name;
    };
    state_string state_strings[] = { { DEVICE_STATE_ACTIVE, L"Active" },
                                     { DEVICE_STATE_DISABLED, L"Disabled" },
                                     { DEVICE_STATE_NOTPRESENT, L"Not Present" },
                                     { DEVICE_STATE_UNPLUGGED, L"Unplugged" } };

    std::wstring get_state_string(DWORD state)
    {
        std::wstring s;
        wchar_t const *sep = L"";
        for(auto const &state_str : state_strings) {
            if((state_str.state & state) != 0) {
                s = std::format(L"{}{}{}", s, sep, state_str.name);
                sep = L" ";
            }
        }
        return s;
    }

    HRESULT audio_controller::attach_to_default_endpoint()
    {
        std::lock_guard lock(endpoint_mutex);

        ComPtr<IMMDeviceCollection> device_collection;
        HR(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATEMASK_ALL, &device_collection));
        UINT num_devices;
        HR(device_collection->GetCount(&num_devices));
        LOG_DEBUG("There are {} devices", num_devices);
        for(UINT i = 0; i < num_devices; ++i) {
            ComPtr<IMMDevice> device;
            HR(device_collection->Item(i, &device));

            DWORD state;
            HR(device->GetState(&state));

            if((state & DEVICE_STATE_NOTPRESENT) != 0) {
                continue;
            }

            ComPtr<IPropertyStore> property_store;
            HR(device->OpenPropertyStore(STGM_READ, &property_store));
            DWORD prop_count;
            HR(property_store->GetCount(&prop_count));
            PROPVARIANT formfactor_property;
            if(!SUCCEEDED(property_store->GetValue(PKEY_AudioEndpoint_FormFactor, &formfactor_property))) {
                LOG_DEBUG("Device {:3d}: no form factor?", i);
                continue;
            }
            uint32 formfactor = formfactor_property.uintVal;
            if(formfactor != Microphone && formfactor != LineLevel || formfactor > _countof(formfactor_names)) {
                continue;
            }
            PROPVARIANT name_prop;
            if(!SUCCEEDED(property_store->GetValue(PKEY_Device_FriendlyName, &name_prop))) {
                LOG_DEBUG("Device {:3d}: ??", i);
                continue;
            }
            LOG_INFO(L"Device {:3d}: {} {} [{}]", i, name_prop.pwszVal, formfactor_names[formfactor],
                     get_state_string(state));
        }

        HR(enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &audio_endpoint));

        HR(audio_endpoint->Activate(__uuidof(volume_control), CLSCTX_INPROC_SERVER, NULL, (void **)&volume_control));
        HR(volume_control->RegisterControlChangeNotify(this));
        volume_registered = true;
        LOG_INFO("Microphone is attached and enabled");
        return S_OK;
    }


    // ----------------------------------------------------------------------
    //  Stop monitoring the device and release all associated references
    // ----------------------------------------------------------------------

    void audio_controller::detach_from_endpoint()
    {
        std::lock_guard lock(endpoint_mutex);

        if(volume_control != NULL) {
            if(volume_registered) {
                volume_control->UnregisterControlChangeNotify(this);
                volume_registered = false;
            }
            volume_control.Reset();
            LOG_INFO("Detached from microphone");
        }
        audio_endpoint.Reset();
    }

    // ----------------------------------------------------------------------
    //  Call this from the UI thread when the default device changes
    // ----------------------------------------------------------------------

    HRESULT audio_controller::refresh_endpoint()
    {
        detach_from_endpoint();
        HR(attach_to_default_endpoint());
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Implementation of IMMNotificationClient::OnDefaultDeviceChanged
    //
    //  When the user changes the default output device we want to stop monitoring the
    //  former default and start monitoring the new default
    // ----------------------------------------------------------------------

    HRESULT audio_controller::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
    {
        if(flow == eCapture && role == eCommunications && overlay_hwnd != nullptr) {
            PostMessage(overlay_hwnd, WM_APP_ENDPOINT_CHANGE, 0, 0);
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  Implementation of IAudioEndpointVolumeCallback::OnNotify
    //
    //  This is called by the audio core when anyone in any process changes the volume or
    //  mute state for the endpoint we are monitoring
    // ----------------------------------------------------------------------

    HRESULT audio_controller::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
    {
        UNREFERENCED_PARAMETER(pNotify);
        if(overlay_hwnd != nullptr) {
            PostMessage(overlay_hwnd, WM_APP_SHOW_OVERLAY, 0, 0);
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------

    HRESULT audio_controller::QueryInterface(REFIID iid, void **ppUnk)
    {
        if((iid == __uuidof(IUnknown)) || (iid == __uuidof(IMMNotificationClient))) {
            *ppUnk = static_cast<IMMNotificationClient *>(this);
        } else if(iid == __uuidof(IAudioEndpointVolumeCallback)) {
            *ppUnk = static_cast<IAudioEndpointVolumeCallback *>(this);
        } else {
            *ppUnk = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    // ----------------------------------------------------------------------

    ULONG audio_controller::AddRef()
    {
        return InterlockedIncrement(&ref_count);
    }

    // ----------------------------------------------------------------------

    ULONG audio_controller::Release()
    {
        long lRef = InterlockedDecrement(&ref_count);
        if(lRef == 0) {
            delete this;
        }
        return lRef;
    }
}
