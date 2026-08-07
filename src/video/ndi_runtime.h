#pragma once

#include <QString>

#ifdef __MIXXXXX_NDI__
#include "mixxxxx_ndi_headers.h"
#endif

namespace mixxx {

// GPL-clean NDI runtime: dynamic load of the proprietary NDI redistributable.
// No compile-time or link-time dependency on NDI SDK libraries.
class NdiRuntime {
  public:
    static NdiRuntime& instance();

    bool tryLoad();
    bool isLoaded() const;
    const NDIlib_v5* api() const;
    QString lastError() const;
    QString redistUrl();

    bool acquire();
    void release();

  private:
    NdiRuntime() = default;
    ~NdiRuntime();

    NdiRuntime(const NdiRuntime&) = delete;
    NdiRuntime& operator=(const NdiRuntime&) = delete;

    bool loadLibrary();
    void unloadLibrary();

    const NDIlib_v5* m_pApi = nullptr;
    QString m_lastError;
    int m_refCount = 0;

#ifdef _WIN32
    void* m_hModule = nullptr;
#else
    void* m_hModule = nullptr;
#endif
};

} // namespace mixxx
