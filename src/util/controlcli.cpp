#include "util/controlcli.h"

#include <stdio.h>

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <algorithm>

#include "control/control.h"
#include "control/controlobject.h"
#include "library/dao/playlistdao.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/trackset/crate/crate.h"
#include "mixer/playermanager.h"
#include "preferences/configobject.h"
#include "track/trackid.h"
#include "util/logging.h"

namespace mixxx {
namespace {

QHash<int, QString> s_videoOverrides;

QString unquotePath(const QString& raw) {
    QString path = raw.trimmed();
    if (path.length() >= 2 &&
            ((path.startsWith('"') && path.endsWith('"')) ||
                    (path.startsWith('\'') && path.endsWith('\'')))) {
        path = path.mid(1, path.length() - 2);
    }
    return path;
}

bool parseDeckNumber(const QString& token, int* pDeck, QString* pError) {
    bool ok = false;
    const int deck = token.trimmed().toInt(&ok);
    if (!ok || deck < 1) {
        if (pError) {
            *pError = QStringLiteral("invalid deck number: %1").arg(token);
        }
        return false;
    }
    *pDeck = deck;
    return true;
}

} // namespace

bool ControlCli::parseSetControlAssignment(
        const QString& assignment,
        QString* pGroup,
        QString* pItem,
        double* pValue,
        QString* pError) {
    const QString trimmed = assignment.trimmed();
    if (!trimmed.startsWith('[')) {
        if (pError) {
            *pError = QStringLiteral("expected [Group],item=value, got: %1").arg(trimmed);
        }
        return false;
    }

    const int groupEnd = trimmed.indexOf(QStringLiteral("],"));
    if (groupEnd < 0) {
        if (pError) {
            *pError = QStringLiteral("missing '],': %1").arg(trimmed);
        }
        return false;
    }

    const int equals = trimmed.indexOf('=', groupEnd);
    if (equals < 0) {
        if (pError) {
            *pError = QStringLiteral("missing '=': %1").arg(trimmed);
        }
        return false;
    }

    *pGroup = trimmed.left(groupEnd + 1);
    *pItem = trimmed.mid(groupEnd + 2, equals - groupEnd - 2).trimmed();
    const QString valueStr = trimmed.mid(equals + 1).trimmed();

    bool ok = false;
    *pValue = valueStr.toDouble(&ok);
    if (!ok) {
        if (pError) {
            *pError = QStringLiteral("value is not numeric: %1").arg(valueStr);
        }
        return false;
    }

    if (pItem->isEmpty()) {
        if (pError) {
            *pError = QStringLiteral("empty control item in: %1").arg(trimmed);
        }
        return false;
    }

    return true;
}

bool ControlCli::applySetControl(
        const QString& group,
        const QString& item,
        double value,
        QString* pError) {
    const ConfigKey key(group, item);
    ControlObject* pControl = ControlObject::getControl(key, ControlFlag::AllowMissingOrInvalid);
    if (pControl == nullptr) {
        if (pError) {
            *pError = QStringLiteral("unknown control: %1,%2").arg(group, item);
        }
        return false;
    }
    pControl->set(value);
    return true;
}

bool ControlCli::applySetControlAssignment(
        const QString& assignment,
        QString* pError) {
    QString group;
    QString item;
    double value = 0.0;
    if (!parseSetControlAssignment(assignment, &group, &item, &value, pError)) {
        return false;
    }
    return applySetControl(group, item, value, pError);
}

void ControlCli::applySetControlAssignments(const QStringList& assignments) {
    for (const QString& assignment : assignments) {
        QString error;
        if (!applySetControlAssignment(assignment, &error)) {
            qWarning() << "CLI set-control:" << error;
        }
    }
}

int ControlCli::dumpControlsToStdout() {
    struct ControlRow {
        QString group;
        QString item;
        double value;
    };

    QList<ControlRow> rows;
    QSet<ConfigKey> seen;

    const QList<QSharedPointer<ControlDoublePrivate>> controls =
            ControlDoublePrivate::getAllInstances();
    for (const QSharedPointer<ControlDoublePrivate>& pControl : controls) {
        if (!pControl) {
            continue;
        }
        const ConfigKey key = pControl->getKey();
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        rows.append({key.group, key.item, pControl->get()});
    }

    std::sort(rows.begin(), rows.end(), [](const ControlRow& a, const ControlRow& b) {
        const int groupCmp = QString::compare(a.group, b.group, Qt::CaseInsensitive);
        if (groupCmp != 0) {
            return groupCmp < 0;
        }
        return QString::compare(a.item, b.item, Qt::CaseInsensitive) < 0;
    });

    for (const ControlRow& row : rows) {
        printf("%s,%s=%.6g\n",
                row.group.toLocal8Bit().constData(),
                row.item.toLocal8Bit().constData(),
                row.value);
    }
    return rows.size();
}

void ControlCli::setVideoOverride(int deck, const QString& path) {
    s_videoOverrides.insert(deck, path);
}

QString ControlCli::videoOverrideForDeck(int deck) {
    return s_videoOverrides.value(deck);
}

void ControlCli::clearVideoOverrides() {
    s_videoOverrides.clear();
}

bool ControlCli::executeGigScriptLine(
        const QString& line,
        int lineNumber,
        PlayerManager* pPlayerManager,
        TrackCollectionManager* pTrackCollectionManager,
        QString* pError) {
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#')) {
        return true;
    }

    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    const QStringList parts = trimmed.split(whitespace, Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return true;
    }

    const QString command = parts.at(0).toLower();

    if (command == QStringLiteral("set")) {
        if (parts.size() < 2) {
            if (pError) {
                *pError = QStringLiteral("line %1: set requires [Group],item=value").arg(lineNumber);
            }
            return false;
        }
        return applySetControlAssignment(parts.at(1), pError);
    }

    if (command == QStringLiteral("load")) {
        if (parts.size() < 3) {
            if (pError) {
                *pError = QStringLiteral("line %1: load requires deck and path").arg(lineNumber);
            }
            return false;
        }
        int deck = 0;
        if (!parseDeckNumber(parts.at(1), &deck, pError)) {
            if (pError) {
                *pError = QStringLiteral("line %1: %2").arg(lineNumber).arg(*pError);
            }
            return false;
        }
        if (pPlayerManager == nullptr) {
            if (pError) {
                *pError = QStringLiteral("line %1: player manager unavailable").arg(lineNumber);
            }
            return false;
        }
        const QString path = unquotePath(parts.mid(2).join(' '));
        pPlayerManager->slotLoadToDeck(path, deck);
        return true;
    }

    if (command == QStringLiteral("video")) {
        if (parts.size() < 3) {
            if (pError) {
                *pError = QStringLiteral("line %1: video requires deck and path").arg(lineNumber);
            }
            return false;
        }
        int deck = 0;
        if (!parseDeckNumber(parts.at(1), &deck, pError)) {
            if (pError) {
                *pError = QStringLiteral("line %1: %2").arg(lineNumber).arg(*pError);
            }
            return false;
        }
        setVideoOverride(deck, unquotePath(parts.mid(2).join(' ')));
        return true;
    }

    if (command == QStringLiteral("queue-crate")) {
        if (parts.size() < 2) {
            if (pError) {
                *pError = QStringLiteral("line %1: queue-crate requires a crate name").arg(lineNumber);
            }
            return false;
        }
        if (pTrackCollectionManager == nullptr ||
                pTrackCollectionManager->internalCollection() == nullptr) {
            if (pError) {
                *pError = QStringLiteral("line %1: library unavailable").arg(lineNumber);
            }
            return false;
        }

        const QString crateName = unquotePath(parts.mid(1).join(' '));
        Crate crate;
        if (!pTrackCollectionManager->internalCollection()->crates().readCrateByName(
                    crateName, &crate)) {
            if (pError) {
                *pError = QStringLiteral("line %1: crate not found: %2")
                                  .arg(lineNumber)
                                  .arg(crateName);
            }
            return false;
        }

        QList<TrackId> trackIds;
        auto result = pTrackCollectionManager->internalCollection()->crates().selectCrateTracksSorted(
                crate.getId());
        while (result.next()) {
            trackIds.append(result.trackId());
        }

        if (trackIds.isEmpty()) {
            qWarning() << "Gig script line" << lineNumber << ": crate is empty:" << crateName;
            return true;
        }

        pTrackCollectionManager->internalCollection()->getPlaylistDAO().addTracksToAutoDJQueue(
                trackIds,
                PlaylistDAO::AutoDJSendLoc::BOTTOM);
        return true;
    }

    if (command == QStringLiteral("autodj")) {
        ControlObject::set(ConfigKey("[AutoDJ]", "enabled"), 1.0);
        return true;
    }

    if (pError) {
        *pError = QStringLiteral("line %1: unknown command '%2'").arg(lineNumber).arg(command);
    }
    return false;
}

bool ControlCli::executeGigScript(
        const QString& scriptPath,
        PlayerManager* pPlayerManager,
        TrackCollectionManager* pTrackCollectionManager,
        QString* pError) {
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (pError) {
            *pError = QStringLiteral("cannot open gig script: %1").arg(scriptPath);
        }
        return false;
    }

    clearVideoOverrides();

    QTextStream stream(&file);
    int lineNumber = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        ++lineNumber;
        QString lineError;
        if (!executeGigScriptLine(
                    line,
                    lineNumber,
                    pPlayerManager,
                    pTrackCollectionManager,
                    &lineError)) {
            if (pError) {
                *pError = lineError;
            }
            return false;
        }
    }

    qInfo().noquote() << "Gig script applied:" << scriptPath;
    return true;
}

} // namespace mixxx
