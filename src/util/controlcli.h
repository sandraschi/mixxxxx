#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class PlayerManager;
class TrackCollectionManager;

namespace mixxx {

class ControlCli {
  public:
    /// Parse "[Group],item=value" into a ConfigKey assignment.
    static bool parseSetControlAssignment(
            const QString& assignment,
            QString* pGroup,
            QString* pItem,
            double* pValue,
            QString* pError = nullptr);

    /// Set a ControlObject by group/item name. Returns false if missing or invalid.
    static bool applySetControl(
            const QString& group,
            const QString& item,
            double value,
            QString* pError = nullptr);

    static bool applySetControlAssignment(
            const QString& assignment,
            QString* pError = nullptr);

    static void applySetControlAssignments(const QStringList& assignments);

    /// Print every registered ControlObject as group,item=value (sorted).
    static int dumpControlsToStdout();

    /// Execute a gig script after CoreServices::initialize().
    static bool executeGigScript(
            const QString& scriptPath,
            PlayerManager* pPlayerManager,
            TrackCollectionManager* pTrackCollectionManager,
            QString* pError = nullptr);

    /// Optional per-deck video path override (1-based deck index).
    static void setVideoOverride(int deck, const QString& path);
    static QString videoOverrideForDeck(int deck);
    static void clearVideoOverrides();

  private:
    static bool executeGigScriptLine(
            const QString& line,
            int lineNumber,
            PlayerManager* pPlayerManager,
            TrackCollectionManager* pTrackCollectionManager,
            QString* pError);
};

} // namespace mixxx
