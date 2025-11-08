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

    // Set up tool button-action mapping
    connect(ui->btnChooseFile,
            &QPushButton::clicked,
            this,
            &MainWindow::choose_input_file);
    connect(ui->btnChooseSoundfont,
            &QPushButton::clicked,
            this,
            &MainWindow::choose_soundfont);

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
        set_soundfont(QString::fromStdString(cfg.soundfont_path));
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

    default_item->setData(0.1, toint(ChannelArgRole::TriggerThreshold));
    default_item->setData(30, toint(ChannelArgRole::MaxNudgeMs));
    default_item->setData(1.0, toint(ChannelArgRole::SimilarityBias));
    default_item->setData(20, toint(ChannelArgRole::SimilarityWindowMs));
    default_item->setData(0.5, toint(ChannelArgRole::PeakBias));
    default_item->setData(0.9, toint(ChannelArgRole::PeakBiasFactor));

    default_item->setData(true, toint(ChannelArgRole::InheritDefaults));
    m_channel_model.appendRow(default_item);
    reinit_channel_model(16); // DEBUG

    // Per-channel model: model updaters
    ui->cmbChannel->setModel(&m_channel_model);
    connect(ui->chbInheritOpts,
            &QCheckBox::clicked,
            model_updater<bool, ChannelArgRole::InheritDefaults>());

    connect(ui->sbScopeWidth,
            &QSpinBox::valueChanged,
            model_updater<int, ChannelArgRole::ScopeWidthMs>());
    connect(ui->sbMaxNudge,
            &QSpinBox::valueChanged,
            model_updater<int, ChannelArgRole::MaxNudgeMs>());
    connect(ui->dsbAmplification,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::Amplification>());
    connect(ui->dsbTriggerThreshold,
            &QDoubleSpinBox::valueChanged,
            model_updater<int, ChannelArgRole::TriggerThreshold>());
    connect(ui->chbStereo,
            &QCheckBox::clicked,
            model_updater<bool, ChannelArgRole::IsStereo>());
    connect(ui->cpWaveColor,
            &controls::ColorPicker::valueChanged,
            model_updater<QColor, ChannelArgRole::WaveColor>());
    connect(ui->dsbWaveThickness,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::WaveThickness>());
    connect(ui->cpMidlineColor,
            &controls::ColorPicker::valueChanged,
            model_updater<QColor, ChannelArgRole::MidlineColor>());
    connect(ui->dsbMidlineThickness,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::MidlineThickness>());
    connect(ui->chbHMidline,
            &QCheckBox::clicked,
            model_updater<bool, ChannelArgRole::DrawHMidline>());
    connect(ui->chbVMidline,
            &QCheckBox::clicked,
            model_updater<bool, ChannelArgRole::DrawVMidline>());
    connect(ui->dsbSimilarityBias,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::SimilarityBias>());
    connect(ui->sbSimilarityWindow,
            &QSpinBox::valueChanged,
            model_updater<int, ChannelArgRole::SimilarityWindowMs>());
    connect(ui->sbScopeWidth,
            &QSpinBox::valueChanged,
            ui->sbSimilarityWindow,
            &QSpinBox::setMaximum);
    connect(ui->dsbPeakBias,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::PeakBias>());
    connect(ui->dsbPeakThreshold,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::PeakBiasFactor>());

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
    syncUiToModel();
}

void MainWindow::set_ui_state(UiState state) {
    switch (state) {
    case UiState::Editing:
        ui->wgtFileChoosers->setEnabled(true);
        ui->gbChannelOpts->setEnabled(true);
        ui->gbGlobalOpts->setEnabled(true);
        break;
    case UiState::Rendering:
        break;
    case UiState::Canceling:
        break;
    }
}

template<typename T>
void MainWindow::update_model_value(ChannelArgRole role, const T& val) {
    int index = ui->cmbChannel->currentIndex();
    m_channel_model.item(index)->setData(val, toint(role));
}

// -- MainWindow slots --

void MainWindow::debugStart() {
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
        set_soundfont("");
        return;
    }

    QString output_file = QFileDialog::getSaveFileName(this,
                                                       "Render File",
                                                       m_output_file_dir,
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
        .border_color = ui->cpGridlineColor->color().rgb(),
        .border_thickness = ui->dsbGridlineThickness->value(),
        .background_color = ui->cpBackground->color().rgb(),
        .debug_vis = ui->chbDebugVis->isChecked(),
    };

    QList<ChannelArgs> channel_args_list;

    auto default_args = m_channel_model.item(0);
    for (int i = 1; i < m_channel_model.rowCount(); i++) {
        auto args = m_channel_model.item(i);
        if (args->data(toint(ChannelArgRole::InheritDefaults)).toBool()) {
            args = default_args;
        }

        channel_args_list << ChannelArgs{
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

            .max_nudge_ms = args->data(toint(ChannelArgRole::MaxNudgeMs)).toInt(),
            .trigger_threshold = args->data(toint(ChannelArgRole::TriggerThreshold))
                                     .toDouble(),
            .similarity_bias = args->data(toint(ChannelArgRole::SimilarityBias)).toDouble(),
            .similarity_window_ms = args->data(toint(ChannelArgRole::SimilarityWindowMs))
                                        .toInt(),
            .peak_bias = args->data(toint(ChannelArgRole::PeakBias)).toDouble(),
            .peak_bias_factor = args->data(toint(ChannelArgRole::PeakBiasFactor))
                                    .toDouble(),
        };
    }

    emit workerStartRequested(m_input_file,
                              m_input_soundfont,
                              output_file,
                              channel_args_list,
                              global_args);
    ui->btnStopRender->setEnabled(true);
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

    ui->btnStartRender->setEnabled(true);
    ui->btnStopRender->setEnabled(false);
}

void MainWindow::choose_input_file() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Choose File",
                                                    m_input_file_dir,
                                                    "MIDI Files (*.mid *.midi)");
    if (filename.isNull())
        return;

    set_input_file(filename);
}

void MainWindow::set_input_file(const QString& filename) {
    m_input_file = filename;
    m_input_file_dir = QFileInfo(filename).absoluteDir().path();
    ui->leInputFile->setText(filename);

    ui->btnStartRender->setDisabled(m_input_file.isEmpty() || m_input_soundfont.isEmpty());
}

void MainWindow::choose_soundfont() {
    QString soundfont_dir;
    if (!m_input_soundfont.isEmpty()) {
        soundfont_dir = QFileInfo(m_input_soundfont).absoluteDir().path();
    }

    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Choose Soundfont",
                                                    soundfont_dir,
                                                    "Soundfonts (*.sf2 *.sfz)");
    if (filename.isNull())
        return;

    set_soundfont(filename);
}

void MainWindow::set_soundfont(const QString& filename) {
    m_input_soundfont = filename;
    ui->leSoundfont->setText(filename);

    ui->btnStartRender->setDisabled(m_input_file.isEmpty() || m_input_soundfont.isEmpty());
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

void MainWindow::setCurrentChannel(int index) {
    m_current_index = index;
    auto item = m_channel_model.item(m_current_index);
    bool inherit_defaults = item->data(toint(ChannelArgRole::InheritDefaults)).toBool()
                            && m_current_index != 0;

    if (inherit_defaults) {
        resetCurrentToDefault();
    } else {
        syncUiToModel();
    }
}

void MainWindow::resetCurrentToDefault() {
    auto default_item = m_channel_model.item(0)->clone();
    auto current_item = m_channel_model.item(m_current_index);

    default_item->setText(current_item->text());
    int role = toint(ChannelArgRole::InheritDefaults);
    default_item->setData(current_item->data(role), role);

    m_channel_model.setItem(m_current_index, default_item);
    syncUiToModel();
}

void MainWindow::syncUiToModel() {
    auto item = m_channel_model.item(m_current_index);

    ui->chbInheritOpts->setVisible(m_current_index != 0);
    ui->btnResetOpts->setVisible(m_current_index != 0);

    bool inherit_defaults = item->data(toint(ChannelArgRole::InheritDefaults)).toBool()
                            && m_current_index != 0;

    ui->chbStereo->setChecked(item->data(toint(ChannelArgRole::IsStereo)).toBool());
    ui->cpWaveColor->setValue(
        item->data(toint(ChannelArgRole::WaveColor)).value<QColor>());
    ui->dsbWaveThickness->setValue(
        item->data(toint(ChannelArgRole::WaveThickness)).toDouble());
    ui->dsbAmplification->setValue(
        item->data(toint(ChannelArgRole::Amplification)).toDouble());
    ui->sbScopeWidth->setValue(item->data(toint(ChannelArgRole::ScopeWidthMs)).toInt());

    ui->cpMidlineColor->setValue(
        item->data(toint(ChannelArgRole::MidlineColor)).value<QColor>());
    ui->dsbMidlineThickness->setValue(
        item->data(toint(ChannelArgRole::MidlineThickness)).toDouble());
    ui->chbHMidline->setChecked(item->data(toint(ChannelArgRole::DrawHMidline)).toBool());
    ui->chbVMidline->setChecked(item->data(toint(ChannelArgRole::DrawVMidline)).toBool());

    ui->sbMaxNudge->setValue(item->data(toint(ChannelArgRole::MaxNudgeMs)).toInt());
    ui->dsbTriggerThreshold->setValue(
        item->data(toint(ChannelArgRole::TriggerThreshold)).toDouble());
    ui->dsbSimilarityBias->setValue(
        item->data(toint(ChannelArgRole::SimilarityBias)).toDouble());
    ui->sbSimilarityWindow->setValue(
        item->data(toint(ChannelArgRole::SimilarityWindowMs)).toInt());
    ui->dsbPeakBias->setValue(item->data(toint(ChannelArgRole::PeakBias)).toDouble());
    ui->dsbPeakThreshold->setValue(
        item->data(toint(ChannelArgRole::PeakBiasFactor)).toDouble());

    ui->chbInheritOpts->setChecked(inherit_defaults);
    ui->scraChannelOpts->setDisabled(inherit_defaults);
}

void MainWindow::recalcPreview() {}
