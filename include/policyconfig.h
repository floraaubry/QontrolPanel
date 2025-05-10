#pragma once

#include <Unknwn.h> // For IUnknown
#include <mmdeviceapi.h> // For ERole
#include <propidl.h> // For PROPVARIANT
#include <Functiondiscoverykeys_devpkey.h> // For PROPERTYKEY
#include <audioclient.h> // For WAVEFORMATEX

// Define IPolicyConfig interface
struct DECLSPEC_UUID("f8679f50-850a-41cf-9c72-430f290290c8") IPolicyConfig : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(
        PCWSTR,
        WAVEFORMATEX**
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(
        PCWSTR,
        INT,
        WAVEFORMATEX**
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(
        PCWSTR
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(
        PCWSTR,
        WAVEFORMATEX*,
        WAVEFORMATEX*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(
        PCWSTR,
        INT,
        PINT64,
        PINT64
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(
        PCWSTR,
        PINT64
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetShareMode(
        PCWSTR,
        struct DeviceShareMode*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetShareMode(
        PCWSTR,
        struct DeviceShareMode*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PCWSTR,
        const PROPERTYKEY&,
        PROPVARIANT*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
        PCWSTR,
        const PROPERTYKEY&,
        const PROPVARIANT&
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
        PCWSTR wszDeviceId,
        ERole eRole
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
        PCWSTR,
        INT
        ) = 0;
};

// CLSID for IPolicyConfig
class DECLSPEC_UUID("870af99c-171d-4f9e-af0d-e63df40c2bc9") CPolicyConfigClient;

// Define IPolicyConfigVista interface
struct DECLSPEC_UUID("568b9108-44bf-40b4-9006-86afe5b5a620") IPolicyConfigVista : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(
        PCWSTR,
        WAVEFORMATEX**
        ) = 0;  // Not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(
        PCWSTR,
        INT,
        WAVEFORMATEX**
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(
        PCWSTR,
        WAVEFORMATEX*,
        WAVEFORMATEX*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(
        PCWSTR,
        INT,
        PINT64,
        PINT64
        ) = 0;  // Not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(
        PCWSTR,
        PINT64
        ) = 0;  // Not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE GetShareMode(
        PCWSTR,
        struct DeviceShareMode*
        ) = 0;  // Not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE SetShareMode(
        PCWSTR,
        struct DeviceShareMode*
        ) = 0;  // Not available on Windows 7, use method from IPolicyConfig

    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
        PCWSTR,
        const PROPERTYKEY&,
        PROPVARIANT*
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
        PCWSTR,
        const PROPERTYKEY&,
        const PROPVARIANT&
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
        PCWSTR wszDeviceId,
        ERole eRole
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
        PCWSTR,
        INT
        ) = 0;  // Not available on Windows 7, use method from IPolicyConfig
};

// CLSID for IPolicyConfigVista
class DECLSPEC_UUID("294935CE-F637-4E7C-A41B-AB255460B862") CPolicyConfigVistaClient;
