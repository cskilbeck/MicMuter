#pragma once

#include "../common_lib/logger.h"

namespace chs::mic_muter
{
    class audio_controller : IMMNotificationClient, IAudioEndpointVolumeCallback
    {
        LOG_CONTEXT("audio_controller");

        bool endpoint_registered{ false };
        bool volume_registered{ false };
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> audio_endpoint;
        ComPtr<IAudioEndpointVolume> volume_control;
        std::mutex endpoint_mutex;

        long ref_count{ 1 };

        ~audio_controller() = default;    // refcounted object... make the destructor private

        HRESULT attach_to_default_endpoint();
        void detach_from_endpoint();

        IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR name, const PROPERTYKEY property_key);

        IFACEMETHODIMP OnDeviceQueryRemove()
        {
            LOG_INFO("OnDeviceQueryRemove");
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceQueryRemoveFailed()
        {
            LOG_INFO("OnDeviceQueryRemoveFailed");
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceRemovePending()
        {
            LOG_INFO("OnDeviceRemovePending");
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceAdded(LPCWSTR pwstrDeviceId)
        {
            LOG_INFO("OnDeviceAdded");
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceRemoved(LPCWSTR pwstrDeviceId)
        {
            LOG_INFO("OnDeviceRemoved");
            return S_OK;
        }

        IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
        {
            LOG_INFO(L"OnDeviceStateChanged: {} -> {}", pwstrDeviceId, dwNewState);
            return S_OK;
        }

        IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId);
        IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);
        IFACEMETHODIMP QueryInterface(REFIID iid, void **ppUnk);

    public:
        audio_controller() = default;

        HRESULT init();
        void close();
        HRESULT refresh_endpoint();

        HRESULT get_mic_info(bool *present, bool *muted);

        HRESULT toggle_mute();

        // IUnknown
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();
    };
}