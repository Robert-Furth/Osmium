#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QThread>
#include <QVariant>

#include "scoperenderer.h"
#include "workers.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum class ChannelArgRole {
    ScopeWidthMs = Qt::UserRole + 2,
    MaxNudgeMs,
    Amplification,
    TriggerThreshold,
    IsStereo,
    Color,
    Thickness,

    InheritDefaults,
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void reinit_channel_model(int num_channels);

private:
    Ui::MainWindow* ui;
    RenderWorker* m_r_worker;
    QThread m_render_thread;

    QStandardItemModel m_channel_model;
    int m_current_index = -1;

    QString m_input_file;

    template<typename T>
    void update_model_value(ChannelArgRole role, const T& val);

    template<typename T, ChannelArgRole role>
    constexpr auto model_updater() {
        return std::bind(&MainWindow::update_model_value<T>,
                         this,
                         role,
                         std::placeholders::_1);
    }

public slots:
    void debugStart();
    void onWorkerStop(bool, const QString&);

    void setCurrentChannel(int);
    void resetCurrentToDefault();
    void syncUiToModel();
    void recalcPreview();

signals:
    void workerStartRequested(const QStringList& filenames,
                              const QList<ScopeRenderer::ChannelArgs>&,
                              const ScopeRenderer::GlobalArgs&);
    void workerStopRequested();
};

#endif // MAINWINDOW_H
