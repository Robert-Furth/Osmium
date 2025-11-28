#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <type_traits>

#include <QMainWindow>
#include <QStandardItemModel>
#include <QThread>
#include <QVariant>

#include "optionsdialog.h"
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

    ShowInstrumentLabels,
    LabelTemplate,
    LabelFontFamily,
    LabelFontSize,
    LabelFontColor,
    LabelBold,
    LabelItalic,

    TriggerThreshold,
    MaxNudgeMs,
    SimilarityBias,
    SimilarityWindowMs,
    PeakBias,
    PeakThreshold,

    InheritDefaults,
    IsVisible,
};

enum class UiState {
    Editing,
    Resetting,
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

public slots:
    void start_rendering();
    void handle_render_stop(bool, const QString&);

    void set_input_file(const QString&);
    void show_options_dialog();
    void update_options_from_dialog();

    void update_cell_order(int);
    void update_channel_opts_enabled();

    void set_current_channel(int);
    void reset_current_channel();
    void recalc_preview();

private:
    Ui::MainWindow* ui;
    UiState m_state = UiState::Editing;

    RenderWorker* m_r_worker;
    QThread m_render_thread;

    QStandardItemModel m_channel_model;
    int m_current_index = -1;

    QString m_input_file;
    QString m_input_soundfont;
    bool m_use_system_ffmpeg;
    QString m_ffmpeg_path;
    QString m_input_file_dir;
    QString m_output_file_dir;

    OptionsDialog* m_options_dialog;

    template<typename T>
    void update_model_value(ChannelArgRole role, const T& val);

    template<typename T>
    constexpr auto model_updater(ChannelArgRole role);

    /** @brief Returns a function that takes in a `QStandardItem*` and calls `control->*setter`
     *  with the value stored in the `role` role of the item.
     *
     *  @tparam Control The type of the control to call the setter of.
     *  @tparam T       The type of the parameter of the setter method.
     *
     *  @param role    The `ChannelArgRole` of the data to call the setter with.
     *  @param control A pointer to the control to call the setter of.
     *  @param setter  A pointer to a member function of `control`. It should take in 1 argument
     *                 of type T.
     */
    template<typename Control, typename T>
    constexpr auto control_setter(ChannelArgRole role,
                                  Control* control,
                                  void (Control::*setter)(T)) const;

    template<typename Control, typename T>
    void bind_to_model(Control* control,
                       ChannelArgRole role,
                       void (Control::*notifier)(T),
                       void (Control::*setter)(T));

    GlobalArgs create_global_args();
    QList<ChannelArgs> create_channel_args();
    ChannelArgs create_channel_args(QStandardItem*, int);

signals:
    void worker_start_requested(const QString& input_file,
                                const QString& soundfont,
                                const QString& output_file,
                                const QString& ffmpeg_path,
                                const QList<ChannelArgs>&,
                                const GlobalArgs&);

    void current_item_changed(const QStandardItem* item);
};

#endif // MAINWINDOW_H
