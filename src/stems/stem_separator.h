#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

class StemSeparator : public QObject {
    Q_OBJECT
  public:
    explicit StemSeparator(QObject* parent = nullptr);
    ~StemSeparator() override;

    bool loadModel(const QString& modelPath);
    QStringList separate(const QString& inputWav, const QString& outputDir);

    bool isLoaded() const;
    QString lastError() const {
        return m_lastError;
    }

  signals:
    void progress(int percent);
    void complete(bool success, const QStringList& stems);

  private:
    std::vector<float> readWav(const QString& path, int& sampleRate);
    bool writeWav(const QString& path, const std::vector<float>& samples, int sampleRate);
    std::vector<float> processChunk(const std::vector<float>& chunk);

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    QString m_lastError;

    static constexpr int kChunkSize = 204800;
};
