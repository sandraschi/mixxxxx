#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>
#include <memory>

#include "widget/wbasewidget.h"

class ControlProxy;

class WPhaseIndicator : public QWidget, public WBaseWidget {
    Q_OBJECT
  public:
    explicit WPhaseIndicator(const QString& group, QWidget* parent = nullptr);
    ~WPhaseIndicator() override;

    void Init() override;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private slots:
    void updatePhase();

  private:
    QString m_group;
    std::unique_ptr<ControlProxy> m_pPhase;
    QTimer* m_pTimer;
    float m_phase = 0.0f;
    QElapsedTimer m_elapsed;
};
