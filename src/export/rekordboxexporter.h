#pragma once
#include <QObject>
#include <QString>

class RekordboxExporter : public QObject {
    Q_OBJECT
  public:
    explicit RekordboxExporter(const QString& settingsPath, QObject* parent = nullptr);

    bool exportTrack(const QString& trackPath, const QString& usbPath);
    bool exportCrate(const QString& crateName, const QString& usbPath);

  signals:
    void exportProgress(int current, int total);
    void exportComplete(bool success, const QString& message);

  private:
    QString m_settingsPath;
};
