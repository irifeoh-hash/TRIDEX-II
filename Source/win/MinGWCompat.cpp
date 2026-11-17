// MinGW-only shims for JUCE 8.0.4 (compiled only when cross-building with MinGW).
// A few JUCE call sites take __uuidof() of a ComSmartPtr<> wrapper rather than of the
// interface it holds. MSVC resolves that to the wrapped interface; MinGW needs the
// __mingw_uuidof specialisations spelled out.

#if defined (__MINGW32__)

#include <juce_core/juce_core.h>
#include <dxgi.h>
#include <juce_core/native/juce_ComSmartPtr_windows.h>

template <> const GUID& __mingw_uuidof<juce::ComSmartPtr<IDXGIDevice>>()  { return __uuidof (IDXGIDevice); }
template <> const GUID& __mingw_uuidof<juce::ComSmartPtr<IDXGISurface>>() { return __uuidof (IDXGISurface); }

#endif
