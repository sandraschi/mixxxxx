#include "video/ndi_runtime.h"

#include <cstdlib>

#include <QByteArray>
#include <QDir>
#include <QFileInfo>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "mixxxxx_ndi_headers.h"

namespace mixxx {
namespace {

constexpr const char* kRedistUrlV5 = "https://ndi.link/NDIRedistV5";
constexpr const char* kRedistUrlV6 = "https://ndi.link/NDIRedistV6";

QString envRuntimeDir() {
    const char* runtimeDir = std::getenv("NDI_RUNTIME_DIR_V5");
    if (runtimeDir == nullptr || runtimeDir[0] == '\0') {
        runtimeDir = std::getenv("NDI_RUNTIME_DIR_V6");
    }
    if (runtimeDir == nullptr || runtimeDir[0] == '\0') {
        return QString();
    }
    return QDir::fromNativeSeparators(QString::fromLocal8Bit(runtimeDir));
}

QString libraryFileName() {
#ifdef _WIN64
    return QStringLiteral("Processing.NDI.Lib.x64.dll");
#elif defined(_WIN32)
    return QStringLiteral("Processing.NDI.Lib.x86.dll");
#elif defined(__APPLE__)
    return QStringLiteral("libndi.dylib");
#else
    return QStringLiteral("libndi.so.6");
#endif
}

using NdiLoadFn = const NDIlib_v5* (*)();

} // namespace

NdiRuntime& NdiRuntime::instance() {
    static NdiRuntime runtime;
    return runtime;
}

NdiRuntime::~NdiRuntime() {
    unloadLibrary();
}

QString NdiRuntime::redistUrl() {
    return QString::fromLatin1(kRedistUrlV5);
}

QString NdiRuntime::lastError() const {
    return m_lastError;
}

bool NdiRuntime::isLoaded() const {
    return m_pApi != nullptr;
}

const NDIlib_v5* NdiRuntime::api() const {
    return m_pApi;
}

bool NdiRuntime::tryLoad() {
    if (m_pApi != nullptr) {
        return true;
    }
    return loadLibrary();
}

bool NdiRuntime::acquire() {
    if (!tryLoad()) {
        return false;
    }
    if (m_refCount == 0) {
        if (!m_pApi->initialize()) {
            m_lastError = QStringLiteral(
                    "NDI runtime loaded but NDIlib initialize() failed (CPU unsupported?)");
            unloadLibrary();
            return false;
        }
    }
    m_refCount++;
    return true;
}

void NdiRuntime::release() {
    if (m_refCount <= 0) {
        return;
    }
    if (--m_refCount == 0 && m_pApi != nullptr) {
        m_pApi->destroy();
    }
}

bool NdiRuntime::loadLibrary() {
    if (m_hModule != nullptr) {
        return m_pApi != nullptr;
    }

    const QString runtimeDir = envRuntimeDir();
    if (runtimeDir.isEmpty()) {
        m_lastError = QStringLiteral(
                "NDI runtime not found. Set NDI_RUNTIME_DIR_V5 (or NDI_RUNTIME_DIR_V6) to the "
                "folder containing %1, or install the NDI redistributable from %2")
                        .arg(libraryFileName(), redistUrl());
        return false;
    }

    const QString libraryPath =
            QDir(runtimeDir).filePath(libraryFileName());
    if (!QFileInfo::exists(libraryPath)) {
        m_lastError = QStringLiteral(
                "NDI runtime directory is set but %1 was not found. Install the NDI "
                "redistributable from %2")
                        .arg(libraryPath, redistUrl());
        return false;
    }

#ifdef _WIN32
    m_hModule = LoadLibraryW(reinterpret_cast<LPCWSTR>(libraryPath.utf16()));
    if (m_hModule == nullptr) {
        m_lastError = QStringLiteral("LoadLibrary failed for %1 (error %2)")
                              .arg(libraryPath)
                              .arg(GetLastError());
        return false;
    }

    NdiLoadFn loadFn = reinterpret_cast<NdiLoadFn>(
            GetProcAddress(static_cast<HMODULE>(m_hModule), "NDIlib_v5_load"));
    if (loadFn == nullptr) {
        loadFn = reinterpret_cast<NdiLoadFn>(
                GetProcAddress(static_cast<HMODULE>(m_hModule), "NDIlib_v6_load"));
    }
#else
    const QByteArray pathBytes = libraryPath.toLocal8Bit();
    m_hModule = dlopen(pathBytes.constData(), RTLD_LOCAL | RTLD_LAZY);
    if (m_hModule == nullptr) {
        m_lastError = QStringLiteral("dlopen failed for %1: %2")
                              .arg(libraryPath, QString::fromLocal8Bit(dlerror()));
        return false;
    }

    NdiLoadFn loadFn = reinterpret_cast<NdiLoadFn>(dlsym(m_hModule, "NDIlib_v5_load"));
    if (loadFn == nullptr) {
        loadFn = reinterpret_cast<NdiLoadFn>(dlsym(m_hModule, "NDIlib_v6_load"));
    }
#endif

    if (loadFn == nullptr) {
        m_lastError = QStringLiteral(
                "NDI library at %1 does not export NDIlib_v5_load / NDIlib_v6_load")
                        .arg(libraryPath);
        unloadLibrary();
        return false;
    }

    m_pApi = loadFn();
    if (m_pApi == nullptr) {
        m_lastError = QStringLiteral("NDIlib load entry point returned null for %1")
                              .arg(libraryPath);
        unloadLibrary();
        return false;
    }

    m_lastError.clear();
    return true;
}

void NdiRuntime::unloadLibrary() {
    if (m_refCount > 0 && m_pApi != nullptr) {
        m_pApi->destroy();
        m_refCount = 0;
    }
    m_pApi = nullptr;

    if (m_hModule == nullptr) {
        return;
    }

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(m_hModule));
#else
    dlclose(m_hModule);
#endif
    m_hModule = nullptr;
}

} // namespace mixxx
