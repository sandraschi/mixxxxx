#pragma once
#include <QObject>
#include <QString>
#include <memory>

class SeratoExporter : public QObject {
    Q_OBJECT
  public:
    explicit SeratoExporter(const QString& settingsPath, QObject* parent = nullptr);

    bool exportLibrary(const QString& exportPath);
    bool exportCrate(const QString& crateName, const QString& exportPath);

  signals:
    void exportProgress(int current, int total);
    void exportComplete(bool success, const QString& message);

  private:
    QString m_settingsPath;
};
