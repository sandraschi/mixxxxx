#pragma once
#include <QObject>
#include <QString>

class VirtualDjExporter : public QObject {
    Q_OBJECT
  public:
    explicit VirtualDjExporter(const QString& settingsPath, QObject* parent = nullptr);

    bool exportLibrary(const QString& exportPath);
    bool exportCrate(const QString& crateName, const QString& exportPath);

  signals:
    void exportProgress(int current, int total);
    void exportComplete(bool success, const QString& message);

  private:
    QString convertKeyToCamelot(const QString& mixxxKey) const;
    QString m_settingsPath;
};
