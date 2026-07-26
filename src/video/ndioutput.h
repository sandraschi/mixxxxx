#pragma once

#include <memory>

#include <QObject>
#include <QTimer>

#include "preferences/usersettings.h"

class ControlPushButton;

namespace mixxx {

// Publishes VideoMixer composite frames as an NDI source (TODO 27).
class NdiOutput : public QObject {
    Q_OBJECT
  public:
    explicit NdiOutput(UserSettingsPointer pConfig, QObject* parent = nullptr);
    ~NdiOutput() override;

    bool sdkAvailable() const;
    bool isSending() const;

  private slots:
    void slotTick();
    void slotEnabledChanged(double value);

  private:
    void startSender();
    void stopSender();
    void sendBlendedFrame();
    QString sourceName() const;

    UserSettingsPointer m_pConfig;
    std::unique_ptr<ControlPushButton> m_pEnabled;
    QTimer m_timer;
    QByteArray m_senderNameUtf8;

#ifdef __MIXXXXX_NDI__
    struct NdiSenderImpl;
    std::unique_ptr<NdiSenderImpl> m_pSender;
#endif
};

} // namespace mixxx
