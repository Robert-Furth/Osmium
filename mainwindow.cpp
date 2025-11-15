#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <filesystem>

#include <QColor>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFontComboBox>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QWaitCondition>

#include "saveload.h"
// #include "scoperenderer.h"
#include "renderargs.h"
#include "workers.h"

// Because typing "static_cast<int>(...)" every time gets old fast
constexpr int toint(ChannelArgRole role) {
    return static_cast<int>(role);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_render_thread(this) {
    ui->setupUi(this);

    // Button group
    ui->bgrpCellOrder->setId(ui->rbColMajorLayout,
                             static_cast<int>(ChannelOrder::COLUMN_MAJOR));
    ui->bgrpCellOrder->setId(ui->rbRowMajorLayout,
                             static_cast<int>(ChannelOrder::ROW_MAJOR));

    // Frame rate dropdown
    ui->cmbFrameRate->addItem("25 fps", 25);
    ui->cmbFrameRate->addItem("30 fps", 30);
    ui->cmbFrameRate->addItem("50 fps", 50);
    ui->cmbFrameRate->addItem("60 fps", 60);
    ui->cmbFrameRate->setCurrentIndex(1);

    // Read in config
    PersistentConfig cfg;
    if (cfg.load()) {
        m_input_file_dir.assign(cfg.input_file_dir);
        m_output_file_dir.assign(cfg.output_file_dir);
        m_input_soundfont.assign(cfg.soundfont_path);
        ui->pcInputFile->set_initial_dir(m_input_file_dir);
    }

    // Per-channel model: default values
    QStandardItem* default_item = new QStandardItem("Default");
    default_item->setData(40, toint(ChannelArgRole::ScopeWidthMs));
    default_item->setData(1.0, toint(ChannelArgRole::Amplification));
    default_item->setData(true, toint(ChannelArgRole::IsStereo));

    default_item->setData(QColor(255, 255, 255), toint(ChannelArgRole::WaveColor));
    default_item->setData(2, toint(ChannelArgRole::WaveThickness));
    default_item->setData(QColor(96, 96, 96), toint(ChannelArgRole::MidlineColor));
    default_item->setData(1, toint(ChannelArgRole::MidlineThickness));
    default_item->setData(true, toint(ChannelArgRole::DrawHMidline));
    default_item->setData(true, toint(ChannelArgRole::DrawVMidline));

    default_item->setData(true, toint(ChannelArgRole::ShowInstrumentLabels));
    default_item->setData(QFont(), toint(ChannelArgRole::LabelFontFamily));
    default_item->setData(QFont().pointSizeF(), toint(ChannelArgRole::LabelFontSize));
    default_item->setData(QColor(255, 255, 255), toint(ChannelArgRole::LabelFontColor));
    default_item->setData(false, toint(ChannelArgRole::LabelBold));
    default_item->setData(false, toint(ChannelArgRole::LabelItalic));

    default_item->setData(0.1, toint(ChannelArgRole::TriggerThreshold));
    default_item->setData(35, toint(ChannelArgRole::MaxNudgeMs));
    default_item->setData(1.0, toint(ChannelArgRole::SimilarityBias));
    default_item->setData(20, toint(ChannelArgRole::SimilarityWindowMs));
    default_item->setData(0.5, toint(ChannelArgRole::PeakBias));
    default_item->setData(0.9, toint(ChannelArgRole::PeakThreshold));

    default_item->setData(true, toint(ChannelArgRole::InheritDefaults));
    default_item->setData(true, toint(ChannelArgRole::IsVisible));
    m_channel_model.appendRow(default_item);

    // Per-channel model: model updaters
    ui->cmbChannel->setModel(&m_channel_model);
    // connect(ui->chbInheritOpts,
    //         &QCheckBox::clicked,
    //         model_updater<bool, ChannelArgRole::InheritDefaults>());
    bind_to_model<QCheckBox, bool>(ui->chbInheritOpts,
                                   ChannelArgRole::InheritDefaults,
                                   &QCheckBox::clicked,
                                   &QCheckBox::setChecked);
    bind_to_model<QCheckBox, bool>(ui->chbIsVisible,
                                   ChannelArgRole::IsVisible,
                                   &QCheckBox::clicked,
                                   &QCheckBox::setChecked);

    bind_to_model(ui->sbScopeWidth,
                  ChannelArgRole::ScopeWidthMs,
                  &QSpinBox::valueChanged,
                  &QSpinBox::setValue);
    bind_to_model(ui->dsbAmplification,
                  ChannelArgRole::Amplification,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model<QCheckBox, bool>(ui->chbStereo,
                                   ChannelArgRole::IsStereo,
                                   &QCheckBox::clicked,
                                   &QCheckBox::setChecked);

    bind_to_model(ui->cpWaveColor,
                  ChannelArgRole::WaveColor,
                  &controls::ColorPicker::colorChanged,
                  &controls::ColorPicker::setColor);
    bind_to_model(ui->dsbWaveThickness,
                  ChannelArgRole::WaveThickness,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model(ui->cpMidlineColor,
                  ChannelArgRole::MidlineColor,
                  &controls::ColorPicker::colorChanged,
                  &controls::ColorPicker::setColor);
    bind_to_model(ui->dsbMidlineThickness,
                  ChannelArgRole::MidlineThickness,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model<QCheckBox, bool>(ui->chbHMidline,
                                   ChannelArgRole::DrawHMidline,
                                   &QCheckBox::clicked,
                                   &QCheckBox::setChecked);
    bind_to_model<QCheckBox, bool>(ui->chbVMidline,
                                   ChannelArgRole::DrawVMidline,
                                   &QCheckBox::clicked,
                                   &QCheckBox::setChecked);

    bind_to_model<QCheckBox, bool>(ui->chbShowLabels,
                                   ChannelArgRole::ShowInstrumentLabels,
                                   &QCheckBox::clicked,
                                   &QCheckBox::setChecked);
    bind_to_model(ui->fcbLabelFont,
                  ChannelArgRole::LabelFontFamily,
                  &QFontComboBox::currentFontChanged,
                  &QFontComboBox::setCurrentFont);
    bind_to_model(ui->dsbFontSize,
                  ChannelArgRole::LabelFontSize,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model(ui->cpFontColor,
                  ChannelArgRole::LabelFontColor,
                  &controls::ColorPicker::colorChanged,
                  &controls::ColorPicker::setColor);
    bind_to_model<QToolButton, bool>(ui->tbLabelBold,
                                     ChannelArgRole::LabelBold,
                                     &QToolButton::clicked,
                                     &QToolButton::setChecked);
    bind_to_model<QToolButton, bool>(ui->tbLabelItalic,
                                     ChannelArgRole::LabelItalic,
                                     &QToolButton::clicked,
                                     &QToolButton::setChecked);

    bind_to_model(ui->dsbTriggerThreshold,
                  ChannelArgRole::TriggerThreshold,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model(ui->sbMaxNudge,
                  ChannelArgRole::MaxNudgeMs,
                  &QSpinBox::valueChanged,
                  &QSpinBox::setValue);
    bind_to_model(ui->dsbSimilarityBias,
                  ChannelArgRole::SimilarityBias,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model(ui->sbSimilarityWindow,
                  ChannelArgRole::SimilarityWindowMs,
                  &QSpinBox::valueChanged,
                  &QSpinBox::setValue);
    bind_to_model(ui->dsbPeakBias,
                  ChannelArgRole::PeakBias,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);
    bind_to_model(ui->dsbPeakThreshold,
                  ChannelArgRole::PeakThreshold,
                  &QDoubleSpinBox::valueChanged,
                  &QDoubleSpinBox::setValue);

    reinit_channel_model(16); // DEBUG

    // Some other deferred connections
    connect(ui->chbIsVisible,
            &QCheckBox::clicked,
            this,
            &MainWindow::update_channel_opts_enabled);
    connect(ui->chbInheritOpts,
            &QCheckBox::clicked,
            this,
            &MainWindow::update_channel_opts_enabled);

    connect(m_options_dialog,
            &OptionsDialog::accepted,
            this,
            &MainWindow::update_options_from_dialog);

    // Set up render thread
    m_r_worker = new RenderWorker();
    m_r_worker->moveToThread(&m_render_thread);

    connect(this, &MainWindow::workerStartRequested, m_r_worker, &RenderWorker::work);
    connect(ui->btnStopRender,
            &QPushButton::clicked,
            m_r_worker,
            &RenderWorker::request_stop);
    connect(m_r_worker->video_worker(),
            &VideoSocketWorker::progress_changed,
            ui->progressBar,
            &QProgressBar::setValue);
    connect(m_r_worker->video_worker(),
            &VideoSocketWorker::preview_image_changed,
            ui->lbCanvas,
            &controls::ResizableQLabel::setPixmap);
    connect(m_r_worker, &RenderWorker::done, this, &MainWindow::onWorkerStop);

    m_render_thread.start();
}

MainWindow::~MainWindow() {
    delete ui;

    PersistentConfig cfg{
        .soundfont_path = m_input_soundfont.toStdString(),
        .input_file_dir = m_input_file_dir.toStdString(),
        .output_file_dir = m_output_file_dir.toStdString(),
    };
    cfg.save();

    m_render_thread.quit();
    m_render_thread.wait();
}

void MainWindow::reinit_channel_model(int num_channels) {
    m_channel_model.setRowCount(num_channels + 1);

    auto default_item = m_channel_model.item(0);
    for (int i = 1; i <= num_channels; i++) {
        auto item = default_item->clone();
        item->setText(QString("Channel %1").arg(i));
        m_channel_model.setItem(i, item);
    }

    m_current_index = 0;
    emit currentItemChanged(m_channel_model.item(m_current_index));
    update_channel_opts_enabled();
    // syncUiToModel();
}

void MainWindow::set_ui_state(UiState state) {
    m_state = state;
    switch (state) {
    case UiState::Editing:
        ui->wgtFileChoosers->setEnabled(true);
        ui->gbChannelOpts->setEnabled(true);
        ui->gbGlobalOpts->setEnabled(true);

        ui->btnStartRender->setEnabled(!m_input_file.isEmpty()
                                       && !m_input_soundfont.isEmpty());
        ui->btnStopRender->setEnabled(false);
        break;
    case UiState::Rendering:
        ui->btnStartRender->setEnabled(false);
        ui->btnStopRender->setEnabled(true);
        break;
    case UiState::Canceling:
        ui->btnStartRender->setEnabled(false);
        ui->btnStopRender->setEnabled(false);
        break;
    }
}

// -- MainWindow slots --

template<typename T>
void MainWindow::update_model_value(ChannelArgRole role, const T& val) {
    int index = ui->cmbChannel->currentIndex();
    m_channel_model.item(index)->setData(val, toint(role));
}

void MainWindow::debugStart() {
    if (m_state != UiState::Editing)
        return;

    if (m_input_file.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please choose a file to render.");
        return;
    }

    if (m_input_soundfont.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please choose a soundfont to use.");
        return;
    }

    if (!std::filesystem::is_regular_file(m_input_file.toStdString())) {
        QString message = QString("The file \"%1\" does not exist or is not a regular "
                                  "file. Please choose a new file.")
                              .arg(m_input_file);
        QMessageBox::warning(this, "Error", message);
        set_input_file("");
        return;
    }

    if (!std::filesystem::is_regular_file(m_input_soundfont.toStdString())) {
        QString message = QString("The file \"%1\" does not exist or is not a regular "
                                  "file. Please choose a new file.")
                              .arg(m_input_soundfont);
        QMessageBox::warning(this, "Error", message);
        return;
    }

    auto outfile_name
        = std::filesystem::path(m_input_file.toStdString()).stem().concat(".mp4");
    auto outfile_path = std::filesystem::path(m_output_file_dir.toStdString())
                        / outfile_name;

    QString output_file = QFileDialog::getSaveFileName(this,
                                                       "Render File",
                                                       QString(outfile_path.c_str()),
                                                       "MP4 Files (*.mp4)");
    if (output_file.isNull())
        return;
    m_output_file_dir = QFileInfo(output_file).absoluteDir().path();

    ui->btnStartRender->setEnabled(false);

    // Set up args
    auto channel_order = static_cast<ChannelOrder>(ui->bgrpCellOrder->checkedId());

    GlobalArgs global_args{
        .width = ui->sbRenderWidth->value(),
        .height = ui->sbRenderHeight->value(),
        .num_rows_or_cols = ui->sbRowColCount->value(),
        .order = channel_order,
        .fps = ui->cmbFrameRate->currentData().toInt(),
        .volume = ui->slVolume->value() / 100.0,
        .border_color = ui->cpGridlineColor->color().rgb(),
        .border_thickness = ui->dsbGridlineThickness->value(),
        .background_color = ui->cpBackground->color().rgb(),
        .debug_vis = ui->chbDebugVis->isChecked(),
    };

    QList<ChannelArgs> channel_args_list;

    auto default_args = m_channel_model.item(0);
    for (int i = 1; i < m_channel_model.rowCount(); i++) {
        auto args = m_channel_model.item(i);

        if (!args->data(toint(ChannelArgRole::IsVisible)).toBool())
            continue;

        if (args->data(toint(ChannelArgRole::InheritDefaults)).toBool()) {
            args = default_args;
        }

        auto font = args->data(toint(ChannelArgRole::LabelFontFamily)).value<QFont>();
        font.setPointSizeF(args->data(toint(ChannelArgRole::LabelFontSize)).toDouble());
        font.setBold(args->data(toint(ChannelArgRole::LabelBold)).toBool());
        font.setItalic(args->data(toint(ChannelArgRole::LabelItalic)).toBool());

        channel_args_list << ChannelArgs{
            .channel_number = i - 1,
            .scope_width_ms = args->data(toint(ChannelArgRole::ScopeWidthMs)).toInt(),

            .amplification = args->data(toint(ChannelArgRole::Amplification)).toDouble(),
            .is_stereo = args->data(toint(ChannelArgRole::IsStereo)).toBool(),

            .color = args->data(toint(ChannelArgRole::WaveColor)).value<QColor>().rgb(),
            .thickness = args->data(toint(ChannelArgRole::WaveThickness)).toDouble(),
            .midline_color
            = args->data(toint(ChannelArgRole::MidlineColor)).value<QColor>().rgb(),
            .midline_thickness = args->data(toint(ChannelArgRole::MidlineThickness))
                                     .toDouble(),
            .draw_h_midline = args->data(toint(ChannelArgRole::DrawHMidline)).toBool(),
            .draw_v_midline = args->data(toint(ChannelArgRole::DrawVMidline)).toBool(),

            .draw_labels = args->data(toint(ChannelArgRole::ShowInstrumentLabels)).toBool(),
            .label_font = font,
            .label_color
            = args->data(toint(ChannelArgRole::LabelFontColor)).value<QColor>().rgb(),

            .max_nudge_ms = args->data(toint(ChannelArgRole::MaxNudgeMs)).toInt(),
            .trigger_threshold = args->data(toint(ChannelArgRole::TriggerThreshold))
                                     .toDouble(),
            .similarity_bias = args->data(toint(ChannelArgRole::SimilarityBias)).toDouble(),
            .similarity_window_ms = args->data(toint(ChannelArgRole::SimilarityWindowMs))
                                        .toInt(),
            .peak_bias = args->data(toint(ChannelArgRole::PeakBias)).toDouble(),
            .peak_threshold = args->data(toint(ChannelArgRole::PeakThreshold)).toDouble(),
        };
    }

    set_ui_state(UiState::Rendering);
    emit workerStartRequested(m_input_file,
                              m_input_soundfont,
                              output_file,
                              channel_args_list,
                              global_args);
}

void MainWindow::onWorkerStop(bool ok, const QString& message) {
    if (!message.isEmpty()) {
        qDebug() << message;
    }

    if (!ok || !message.isEmpty()) {
        QString display_message;
        if (!ok) {
            display_message = message.isEmpty()
                                  ? "An error occurred during rendering."
                                  : "The following error occurred during rendering:\n"
                                        + message;
        } else {
            display_message = message;
        }
        QString title = ok ? "Osmium" : "Error";
        QMessageBox::Icon icon = ok ? QMessageBox::Information : QMessageBox::Warning;

        QMessageBox mbox(icon,
                         title,
                         display_message,
                         QMessageBox::StandardButton::Ok,
                         this);
        mbox.exec();
    }

    set_ui_state(UiState::Editing);
}

void MainWindow::set_input_file(const QString& filename) {
    m_input_file = filename;
    m_input_file_dir = QFileInfo(filename).absoluteDir().path();
    ui->pcInputFile->set_initial_dir(m_input_file_dir);
    // ui->leInputFile->setText(filename);
    if (ui->pcInputFile->current_path() != filename) {
        ui->pcInputFile->set_current_path(filename);
    }

    ui->btnStartRender->setDisabled(
        m_input_file.isEmpty() /* || m_input_soundfont.isEmpty()*/);
}


}


void MainWindow::update_cell_order(int order) {
    switch (static_cast<ChannelOrder>(order)) {
    case ChannelOrder::COLUMN_MAJOR:
        ui->lbRowColCount->setText("Row Count");
        break;
    case ChannelOrder::ROW_MAJOR:
        ui->lbRowColCount->setText("Column Count");
        break;
    }
}

void MainWindow::update_channel_opts_enabled() {
    if (m_current_index == 0) {
        ui->scraChannelOpts->setEnabled(true);
        return;
    }

    auto item = m_channel_model.item(m_current_index);
    ui->scraChannelOpts->setEnabled(
        !item->data(toint(ChannelArgRole::InheritDefaults)).toBool()
        && item->data(toint(ChannelArgRole::IsVisible)).toBool());
}

void MainWindow::setCurrentChannel(int index) {
    m_current_index = index;
    auto item = m_channel_model.item(m_current_index);
    bool inherit_defaults = item->data(toint(ChannelArgRole::InheritDefaults)).toBool()
                            && m_current_index != 0;

    ui->wgtNonDefaultControls->setVisible(m_current_index != 0);

    if (inherit_defaults) {
        resetCurrentToDefault();
    }

    item = m_channel_model.item(m_current_index);
    emit currentItemChanged(item);
    update_channel_opts_enabled();
}

void MainWindow::resetCurrentToDefault() {
    auto default_item = m_channel_model.item(0)->clone();
    auto current_item = m_channel_model.item(m_current_index);

    default_item->setText(current_item->text());
    default_item->setData(current_item->data(toint(ChannelArgRole::InheritDefaults)),
                          toint(ChannelArgRole::InheritDefaults));
    default_item->setData(current_item->data(toint(ChannelArgRole::IsVisible)),
                          toint(ChannelArgRole::IsVisible));

    m_channel_model.setItem(m_current_index, default_item);
    emit currentItemChanged(default_item);
    // syncUiToModel();
}

void MainWindow::recalcPreview() {}
