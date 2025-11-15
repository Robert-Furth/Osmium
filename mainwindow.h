#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <type_traits>

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

    ShowInstrumentLabels,
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

    template<typename T>
    constexpr auto model_updater(ChannelArgRole role) {
        return std::bind(&MainWindow::update_model_value<T>,
                         this,
                         role,
                         std::placeholders::_1);
    }

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
                                  void (Control::*setter)(T)) const {
        // If T is e.g. a const ref, then item->data(...).value<T> won't compile.
        // So, we have to remove the reference and the const qualifiers.
        using ValT = std::remove_cv_t<std::remove_reference_t<T>>;
        return [role, control, setter](const QStandardItem* item) {
            const auto& value = item->data(static_cast<int>(role)).value<ValT>();
            (control->*setter)(value);
        };
    }

    template<typename Control, typename T>
    void bind_to_model(Control* control,
                       ChannelArgRole role,
                       void (Control::*notifier)(T),
                       void (Control::*setter)(T)) {
        // auto asdf = &MainWindow::debugStart;
        connect(control, notifier, this, model_updater<T>(role));
        connect(this,
                &MainWindow::currentItemChanged,
                control_setter(role, control, setter));
    }

public slots:
    void debugStart();
    void onWorkerStop(bool, const QString&);

    // void choose_input_file();
    void set_input_file(const QString&);
    // void choose_soundfont();
    // void set_soundfont(const QString&);

    void update_cell_order(int);
    void update_channel_opts_enabled();

    void setCurrentChannel(int);
    void resetCurrentToDefault();
    // void syncUiToModel();
    void recalcPreview();

signals:
    void workerStartRequested(const QString& input_file,
                              const QString& soundfont,
                              const QString& output_file,
                              const QList<ChannelArgs>&,
                              const GlobalArgs&);
    void workerStopRequested();

    void currentItemChanged(const QStandardItem* item);
};

#endif // MAINWINDOW_H
