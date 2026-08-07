#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <optional>

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

#include "preferences/usersettings.h"

class ControlPushButton;

namespace mixxx {

#ifdef __MIXXXXX_NDI__
struct NdiPendingFrame {
    QByteArray pixels;
    int width = 0;
    int height = 0;
    int strideBytes = 0;
    int64_t timecode = 0;
};

class NdiSenderWorker : public QThread {
    Q_OBJECT
  public:
    explicit NdiSenderWorker(QObject* parent = nullptr);
    ~NdiSenderWorker() override;

    void setSender(void* senderInstance);
    void enqueueFrame(NdiPendingFrame frame);
    void requestStop();

  protected:
    void run() override;

  private:
    static constexpr int kMaxQueueDepth = 2;

    QMutex m_mutex;
    QWaitCondition m_cond;
    std::deque<NdiPendingFrame> m_queue;
    bool m_stop = false;
    void* m_sender = nullptr;
};
#endif

// Publishes VideoMixer composite frames as an NDI source (TODO 27).
class NdiOutput : public QObject {
    Q_OBJECT
  public:
    explicit NdiOutput(UserSettingsPointer pConfig, QObject* parent = nullptr);
    ~NdiOutput() override;

    bool featureCompiled() const;
    bool runtimeAvailable() const;
    bool isSending() const;
    QString runtimeStatusMessage() const;

  private slots:
    void slotTick();
    void slotEnabledChanged(double value);

  private:
    void startSender();
    void stopSender();
    void captureAndQueueFrame();
    void notifyRuntimeMissingOnce();
    QString sourceName() const;

    UserSettingsPointer m_pConfig;
    std::unique_ptr<ControlPushButton> m_pEnabled;
    QTimer m_timer;
    QByteArray m_senderNameUtf8;
    bool m_runtimeWarned = false;
    int64_t m_frameIndex = 0;

#ifdef __MIXXXXX_NDI__
    struct NdiSenderImpl;
    std::unique_ptr<NdiSenderImpl> m_pSender;
    std::unique_ptr<NdiSenderWorker> m_pWorker;
#endif
};

} // namespace mixxx
