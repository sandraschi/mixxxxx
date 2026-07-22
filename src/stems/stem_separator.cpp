#include "stem_separator.h"

#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QByteArray>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <onnxruntime_cxx_api.h>

struct StemSeparator::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "stem-separator"};
    Ort::Session session{nullptr};
    Ort::MemoryInfo memoryInfo{Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault)};
};

StemSeparator::StemSeparator(QObject* parent)
        : QObject(parent),
          m_impl(std::make_unique<Impl>()) {
}

StemSeparator::~StemSeparator() = default;

bool StemSeparator::isLoaded() const {
    return m_impl && m_impl->session;
}

bool StemSeparator::loadModel(const QString& modelPath) {
    try {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(4);
        sessionOptions.SetGraphOptimizationLevel(
                GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        auto pathW = modelPath.toStdWString();
        m_impl->session = Ort::Session(
                m_impl->env, pathW.c_str(), sessionOptions);
        return true;
    } catch (const Ort::Exception& e) {
        m_lastError = QStringLiteral("ONNX load error: %1").arg(e.what());
        return false;
    }
}

std::vector<float> StemSeparator::readWav(const QString& path, int& sampleRate) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QStringLiteral("Cannot open WAV file: %1").arg(path);
        return {};
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char id[4];
    stream.readRawData(id, 4);
    if (std::memcmp(id, "RIFF", 4) != 0) {
        m_lastError = QStringLiteral("Not a RIFF file: %1").arg(path);
        return {};
    }

    stream.skipRawData(4);
    stream.readRawData(id, 4);
    if (std::memcmp(id, "WAVE", 4) != 0) {
        m_lastError = QStringLiteral("Not a WAVE file: %1").arg(path);
        return {};
    }

    quint16 audioFormat = 0;
    quint16 numChannels = 0;
    quint32 sampleRate32 = 0;
    quint16 bitsPerSample = 0;

    while (!stream.atEnd()) {
        stream.readRawData(id, 4);
        quint32 chunkSize;
        stream >> chunkSize;

        if (std::memcmp(id, "fmt ", 4) == 0) {
            stream >> audioFormat >> numChannels >> sampleRate32;
            stream.skipRawData(6);
            stream >> bitsPerSample;
            // Skip any remaining fmt chunk bytes
            int remaining = static_cast<int>(chunkSize) - 16;
            if (remaining > 0) {
                stream.skipRawData(remaining);
            }
        } else if (std::memcmp(id, "data", 4) == 0) {
            sampleRate = static_cast<int>(sampleRate32);

            int totalSamples = static_cast<int>(chunkSize) / (bitsPerSample / 8);
            std::vector<float> allSamples;
            allSamples.reserve(totalSamples);

            if (bitsPerSample == 16) {
                for (int i = 0; i < totalSamples; ++i) {
                    qint16 s;
                    stream >> s;
                    allSamples.push_back(static_cast<float>(s) / 32768.0f);
                }
            } else if (bitsPerSample == 32 && audioFormat == 3) {
                for (int i = 0; i < totalSamples; ++i) {
                    float s;
                    stream >> s;
                    allSamples.push_back(s);
                }
            } else if (bitsPerSample == 32) {
                for (int i = 0; i < totalSamples; ++i) {
                    qint32 s;
                    stream >> s;
                    allSamples.push_back(static_cast<float>(s) / 2147483648.0f);
                }
            } else {
                m_lastError = QStringLiteral("Unsupported bits per sample: %1").arg(bitsPerSample);
                return {};
            }

            if (numChannels == 2) {
                size_t monoCount = allSamples.size() / 2;
                std::vector<float> mono(monoCount);
                for (size_t i = 0; i < monoCount; ++i) {
                    mono[i] = (allSamples[i * 2] + allSamples[i * 2 + 1]) * 0.5f;
                }
                return mono;
            }
            return allSamples;
        } else {
            stream.skipRawData(chunkSize);
        }
    }

    m_lastError = QStringLiteral("No data chunk found in WAV file");
    return {};
}

bool StemSeparator::writeWav(
        const QString& path,
        const std::vector<float>& samples,
        int sampleRate) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 dataSize = static_cast<quint32>(samples.size()) * 2;
    quint32 fileSize = 36 + dataSize;

    stream.writeRawData("RIFF", 4);
    stream << fileSize;
    stream.writeRawData("WAVE", 4);

    stream.writeRawData("fmt ", 4);
    stream << static_cast<quint32>(16);
    stream << static_cast<quint16>(1);
    stream << static_cast<quint16>(1);
    stream << static_cast<quint32>(sampleRate);
    stream << static_cast<quint32>(sampleRate * 2);
    stream << static_cast<quint16>(2);
    stream << static_cast<quint16>(16);

    stream.writeRawData("data", 4);
    stream << dataSize;

    for (float s : samples) {
        float clamped = std::clamp(s, -1.0f, 1.0f);
        qint16 val = static_cast<qint16>(clamped * 32767.0f);
        stream << val;
    }

    return true;
}

std::vector<float> StemSeparator::processChunk(const std::vector<float>& chunk) {
    auto& env = m_impl->env;
    auto& session = m_impl->session;
    auto& memoryInfo = m_impl->memoryInfo;

    // Pad or truncate to exactly kChunkSize samples
    std::vector<float> padded(static_cast<size_t>(kChunkSize), 0.0f);
    size_t copySize = std::min(static_cast<size_t>(kChunkSize), chunk.size());
    std::copy(chunk.begin(), chunk.begin() + copySize, padded.begin());

    // Duplicate mono to stereo: both channels get the same data
    std::vector<float> input(static_cast<size_t>(2) * kChunkSize);
    std::copy(padded.begin(), padded.end(), input.begin());
    std::copy(padded.begin(), padded.end(), input.begin() + kChunkSize);

    std::vector<int64_t> inputShape = {1, 2, kChunkSize};
    auto inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            input.data(),
            input.size(),
            inputShape.data(),
            inputShape.size());

    const char* inputNames[] = {"input"};
    const char* outputNames[] = {"output"};

    auto outputTensors = session.Run(Ort::RunOptions{nullptr},
            inputNames, &inputTensor, 1,
            outputNames, 1);

    auto* outputData = outputTensors[0].GetTensorMutableData<float>();
    auto outputTypeInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    auto outputShape = outputTypeInfo.GetShape();

    int64_t totalOutput = 1;
    for (auto dim : outputShape) {
        totalOutput *= dim;
    }

    std::vector<float> result(static_cast<size_t>(totalOutput));
    std::copy(outputData, outputData + totalOutput, result.begin());
    return result;
}

QStringList StemSeparator::separate(
        const QString& inputWav, const QString& outputDir) {
    int sampleRate = 0;
    auto audio = readWav(inputWav, sampleRate);
    if (audio.empty()) {
        emit complete(false, {});
        return {};
    }

    std::vector<float> vocals, drums, bass, other;
    int totalChunks = (static_cast<int>(audio.size()) + kChunkSize - 1) / kChunkSize;

    for (int c = 0; c < totalChunks; ++c) {
        int start = c * kChunkSize;
        int size = std::min(kChunkSize,
                static_cast<int>(audio.size()) - start);

        std::vector<float> chunk(
                audio.begin() + start,
                audio.begin() + start + size);
        auto stems = processChunk(chunk);

        if (stems.size() >= static_cast<size_t>(kChunkSize * 4)) {
            vocals.insert(vocals.end(),
                    stems.begin(),
                    stems.begin() + size);
            drums.insert(drums.end(),
                    stems.begin() + static_cast<size_t>(kChunkSize),
                    stems.begin() + static_cast<size_t>(kChunkSize) + size);
            bass.insert(bass.end(),
                    stems.begin() + static_cast<size_t>(kChunkSize) * 2,
                    stems.begin() + static_cast<size_t>(kChunkSize) * 2 + size);
            other.insert(other.end(),
                    stems.begin() + static_cast<size_t>(kChunkSize) * 3,
                    stems.begin() + static_cast<size_t>(kChunkSize) * 3 + size);
        }

        emit progress((c + 1) * 100 / totalChunks);
    }

    QDir().mkpath(outputDir);
    QStringList outFiles = {
            outputDir + QStringLiteral("/vocals.wav"),
            outputDir + QStringLiteral("/drums.wav"),
            outputDir + QStringLiteral("/bass.wav"),
            outputDir + QStringLiteral("/other.wav"),
    };

    writeWav(outFiles[0], vocals, sampleRate);
    writeWav(outFiles[1], drums, sampleRate);
    writeWav(outFiles[2], bass, sampleRate);
    writeWav(outFiles[3], other, sampleRate);

    emit complete(true, outFiles);
    return outFiles;
}
