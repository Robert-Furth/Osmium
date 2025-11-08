#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QThread>
#include <QVariant>

#include "workers.h"

namespace Ui {
class MainWindow;
}

enum class ChannelArgRole {
    ScopeWidthMs = Qt::UserRole + 2,
    Amplification,
    IsStereo,

    WaveColor,
    WaveThickness,
    MidlineColor,
    MidlineThickness,
    DrawHMidline,
    DrawVMidline,

    TriggerThreshold,
    MaxNudgeMs,
    SimilarityBias,
    SimilarityWindowMs,
    PeakBias,
    PeakBiasFactor,

    InheritDefaults,
};

enum class UiState {
    Editing,
    Rendering,
    Canceling,
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void reinit_channel_model(int num_channels);
    void set_ui_state(UiState state);

private:
    Ui::MainWindow* ui;
    UiState m_state = UiState::Editing;

    RenderWorker* m_r_worker;
    QThread m_render_thread;

    QStandardItemModel m_channel_model;
    int m_current_index = -1;

    QString m_input_file;
    QString m_input_soundfont;
    QString m_input_file_dir;
    QString m_output_file_dir;

    template<typename T>
    void update_model_value(ChannelArgRole role, const T& val);

    template<typename T, ChannelArgRole role>
    constexpr inline auto model_updater() {
        return std::bind(&MainWindow::update_model_value<T>,
                         this,
                         role,
                         std::placeholders::_1);
    }

public slots:
    void debugStart();
    void onWorkerStop(bool, const QString&);

    void choose_input_file();
    void set_input_file(const QString&);
    void choose_soundfont();
    void set_soundfont(const QString&);

    void update_cell_order(int);

    void setCurrentChannel(int);
    void resetCurrentToDefault();
    void syncUiToModel();
    void recalcPreview();

signals:
    void workerStartRequested(const QString& input_file,
                              const QString& soundfont,
                              const QString& output_file,
                              const QList<ChannelArgs>&,
                              const GlobalArgs&);
    void workerStopRequested();
};

#endif // MAINWINDOW_H
