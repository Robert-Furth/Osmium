#include "mainwindow.h"
#include "./ui_mainwindow.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <QColor>
#include <QDir>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QWaitCondition>

#include "constants.h"
#include "scoperenderer.h"
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

    // Frame rates
    ui->cmbFrameRate->addItem("25", 25);
    ui->cmbFrameRate->addItem("30", 30);
    ui->cmbFrameRate->addItem("50", 50);
    ui->cmbFrameRate->addItem("60", 60);
    ui->cmbFrameRate->setCurrentIndex(1);

    // Per-channel model: default values
    QStandardItem* default_item = new QStandardItem("Default");
    default_item->setData(40, toint(ChannelArgRole::ScopeWidthMs));
    default_item->setData(16, toint(ChannelArgRole::MaxNudgeMs));
    default_item->setData(1.0, toint(ChannelArgRole::Amplification));
    default_item->setData(0.25, toint(ChannelArgRole::TriggerThreshold));
    default_item->setData(true, toint(ChannelArgRole::IsStereo));
    default_item->setData(QColor(255, 255, 255), toint(ChannelArgRole::Color));
    default_item->setData(2, toint(ChannelArgRole::Thickness));
    default_item->setData(true, toint(ChannelArgRole::InheritDefaults));
    m_channel_model.appendRow(default_item);
    reinit_channel_model(16); // DEBUG

    // Per-channel model: model updaters
    ui->cmbChannel->setModel(&m_channel_model);

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
    connect(ui->dsbWaveThickness,
            &QDoubleSpinBox::valueChanged,
            model_updater<double, ChannelArgRole::Thickness>());
    connect(ui->chbInheritOpts,
            &QCheckBox::clicked,
            model_updater<bool, ChannelArgRole::InheritDefaults>());

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
            &ResizableQLabel::setPixmap);
    connect(m_r_worker, &RenderWorker::done, this, &MainWindow::onWorkerStop);

    m_render_thread.start();
}

MainWindow::~MainWindow() {
    delete ui;
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
}

template<typename T>
void MainWindow::update_model_value(ChannelArgRole role, const T& val) {
    int index = ui->cmbChannel->currentIndex();
    m_channel_model.item(index)->setData(val, toint(role));
}

void MainWindow::debugStart() {
    ui->btnStartRender->setEnabled(false);

    // TODO: implement row-major/column-major
    ScopeRenderer::GlobalArgs global_args{
        .width = ui->sbRenderWidth->value(),
        .height = ui->sbRenderHeight->value(),
        .num_rows_or_cols = ui->sbColCount->value(),
        .order = ScopeRenderer::ChannelOrder::ROW_MAJOR,
        .fps = ui->cmbFrameRate->currentData().toInt(),
        .border_color = qRgb(0, 0x80, 0xa0),
        .border_thickness = 2,
        .debug_vis = ui->chbDebugVis->isChecked(),
    };

    QList<ScopeRenderer::ChannelArgs> channel_args_list;

    auto default_args = m_channel_model.item(0);
    for (int i = 1; i < m_channel_model.rowCount(); i++) {
        auto args = m_channel_model.item(i);
        if (args->data(toint(ChannelArgRole::InheritDefaults)).toBool()) {
            args = default_args;
        }

        channel_args_list << ScopeRenderer::ChannelArgs{
            .scope_width_ms = args->data(toint(ChannelArgRole::ScopeWidthMs)).toInt(),
            .max_nudge_ms = args->data(toint(ChannelArgRole::MaxNudgeMs)).toInt(),
            .amplification = args->data(toint(ChannelArgRole::Amplification)).toDouble(),
            .trigger_threshold = args->data(toint(ChannelArgRole::TriggerThreshold))
                                     .toDouble(),
            .is_stereo = args->data(toint(ChannelArgRole::IsStereo)).toBool(),
            .color = args->data(toint(ChannelArgRole::Color)).value<QColor>().rgb(),
            .thickness = args->data(toint(ChannelArgRole::Thickness)).toDouble(),
        };
    }

    emit workerStartRequested({DEBUG_INPUT_PATH}, channel_args_list, global_args);
}

void MainWindow::onWorkerStop(bool ok, const QString& message) {
    qDebug() << message;
    ui->btnStartRender->setEnabled(true);
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

    ui->sbScopeWidth->setValue(item->data(toint(ChannelArgRole::ScopeWidthMs)).toInt());
    ui->sbMaxNudge->setValue(item->data(toint(ChannelArgRole::MaxNudgeMs)).toInt());
    ui->dsbAmplification->setValue(
        item->data(toint(ChannelArgRole::Amplification)).toDouble());
    ui->dsbTriggerThreshold->setValue(
        item->data(toint(ChannelArgRole::TriggerThreshold)).toDouble());
    ui->chbStereo->setChecked(item->data(toint(ChannelArgRole::IsStereo)).toBool());
    // TODO color
    ui->dsbWaveThickness->setValue(
        item->data(toint(ChannelArgRole::Thickness)).toDouble());
    ui->chbInheritOpts->setChecked(inherit_defaults);
    ui->scraChannelOpts->setDisabled(inherit_defaults);
}

void MainWindow::recalcPreview() {}

#include "mainwindow.moc"
