#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QList>
#include <QString>

#include "util/logging.h"

/// A structure to store the parsed command-line arguments
class CmdlineArgs final {
  public:
    /// The constructor is only public to make this class reusable in tests.
    /// All operational code in Mixxx itself must access the global singleton
    /// via `CmdlineArgs::instance()`.
    CmdlineArgs();

    static inline CmdlineArgs& Instance() {
        static CmdlineArgs cla;
        return cla;
    }

    //! The original parser that provides the parsed values to Mixxx
    //! This can be called before anything else is initialized
    bool parse(int argc, char** argv);

    //! The optional second run, that provides translated user feedback
    //! This requires an initialized QCoreApplication
    void parseForUserFeedback();

    const QList<QString>& getMusicFiles() const { return m_musicFiles; }
    bool getStartInFullscreen() const { return m_startInFullscreen; }
    bool getStartAutoDJ() const {
        return m_startAutoDJ;
    }
    bool getControllerDebug() const {
        return m_controllerDebug;
    }
    bool getControllerAbortOnWarning() const {
        return m_controllerAbortOnWarning;
    }
    bool getDeveloper() const { return m_developer; }
#ifdef MIXXX_USE_QML
    bool isQml() const {
        return m_qml;
    }
#endif
    bool getSafeMode() const { return m_safeMode; }
    bool useColors() const {
        return m_useColors;
    }
    bool getUseLegacyVuMeter() const {
        return m_useLegacyVuMeter;
    }
    bool getUseLegacySpinny() const {
        return m_useLegacySpinny;
    }
    bool getDebugAssertBreak() const { return m_debugAssertBreak; }
    bool getSettingsPathSet() const { return m_settingsPathSet; }
    mixxx::LogLevel getLogLevel() const { return m_logLevel; }
    mixxx::LogLevel getLogFlushLevel() const { return m_logFlushLevel; }
    qint64 getLogMaxFileSize() const {
        return m_logMaxFileSize;
    }
    bool getTimelineEnabled() const { return !m_timelinePath.isEmpty(); }
    const QString& getLocale() const { return m_locale; }
    const QString& getSettingsPath() const { return m_settingsPath; }
    void setSettingsPath(const QString& newSettingsPath) {
        m_settingsPath = newSettingsPath;
    }
    const QString& getResourcePath() const { return m_resourcePath; }
    const QString& getTimelinePath() const { return m_timelinePath; }

    const QString& getStyle() const {
        return m_styleName;
    }

    const QStringList& getSetControls() const {
        return m_setControls;
    }
    const QString& getGigScriptPath() const {
        return m_gigScriptPath;
    }
    bool getDumpControls() const {
        return m_dumpControls;
    }
    bool getNoBanner() const {
        return m_noBanner;
    }

    bool hasExportCrateRequest() const {
        return m_hasExportCrateRequest;
    }
    const QString& getExportCrateName() const {
        return m_exportCrateName;
    }
    const QString& getExportFormat() const {
        return m_exportFormat;
    }
    const QString& getExportPath() const {
        return m_exportPath;
    }

    bool hasImportCrateRequest() const {
        return m_hasImportCrateRequest;
    }
    const QString& getImportCratePath() const {
        return m_importCratePath;
    }
    const QString& getImportIntoCrateName() const {
        return m_importIntoCrateName;
    }

    const QString& getVideoPoolPath() const {
        return m_videoPoolPath;
    }

    bool getNdiEnable() const {
        return m_ndiEnable;
    }

    bool getOscDisabled() const {
        return m_oscDisabled;
    }
    bool getOscPortInSet() const {
        return m_oscPortInSet;
    }
    bool getOscPortOutSet() const {
        return m_oscPortOutSet;
    }
    bool getOscHostOutSet() const {
        return m_oscHostOutSet;
    }
    int getOscPortIn() const {
        return m_oscPortIn;
    }
    int getOscPortOut() const {
        return m_oscPortOut;
    }
    const QString& getOscHostOut() const {
        return m_oscHostOut;
    }

    void setScaleFactor(double scaleFactor) {
        m_scaleFactor = scaleFactor;
    }
    double getScaleFactor() const {
        return m_scaleFactor;
    }

  private:
    enum class ParseMode {
        Initial,
        ForUserFeedback
    };

    bool parse(const QStringList& arguments, ParseMode mode);

    QList<QString> m_musicFiles;    // List of files to load into players at startup
    bool m_startInFullscreen;       // Start in fullscreen mode
    bool m_startAutoDJ;
    bool m_controllerDebug;
    bool m_controllerAbortOnWarning; // Controller Engine will be stricter
    bool m_developer; // Developer Mode
#ifdef MIXXX_USE_QML
    bool m_qml;
#endif
    bool m_safeMode;
    bool m_useLegacyVuMeter;
    bool m_useLegacySpinny;
    bool m_debugAssertBreak;
    bool m_settingsPathSet; // has --settingsPath been set on command line ?
    double m_scaleFactor;
    bool m_useColors;       // should colors be used
    bool m_parseForUserFeedbackRequired;
    mixxx::LogLevel m_logLevel; // Level of stderr logging message verbosity
    mixxx::LogLevel m_logFlushLevel; // Level of mixx.log file flushing
    qint64 m_logMaxFileSize;
    QString m_locale;
    QString m_settingsPath;
    QString m_resourcePath;
    QString m_timelinePath;
    QString m_styleName;
    QStringList m_setControls;
    QString m_gigScriptPath;
    bool m_dumpControls;
    bool m_noBanner;
    bool m_hasExportCrateRequest;
    QString m_exportCrateName;
    QString m_exportFormat;
    QString m_exportPath;
    bool m_hasImportCrateRequest;
    QString m_importCratePath;
    QString m_importIntoCrateName;
    QString m_videoPoolPath;
    bool m_ndiEnable = false;
    bool m_oscDisabled = false;
    bool m_oscPortInSet = false;
    bool m_oscPortOutSet = false;
    bool m_oscHostOutSet = false;
    int m_oscPortIn = 11119;
    int m_oscPortOut = 11118;
    QString m_oscHostOut;
};
