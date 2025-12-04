#include "optionsdialog.h"
#include "ui_optionsdialog.h"

#include <QDir>
#include <QDirListing>
#include <QtSystemDetection>

#include "config.h"

OptionsDialog::OptionsDialog(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::OptionsDialog),
      m_env(QProcessEnvironment::systemEnvironment()) {
    ui->setupUi(this);

    setWindowTitle("Options");

#ifdef Q_OS_WINDOWS
    ui->pcFfmpegPath->set_filter("Executables (*.exe);;All files (*.*)");
#endif

    ui->lbFfmpegFound->hide();
    ui->lbFfmpegNotFound->hide();

    m_timer.setInterval(5000);
    connect(&m_timer, &QTimer::timeout, this, &OptionsDialog::check_path_for_ffmpeg);
}

OptionsDialog::~OptionsDialog() {
    delete ui;
}

void OptionsDialog::showEvent(QShowEvent* e) {
    QDialog::showEvent(e);
    enable_ffmpeg_check_timer(ui->rbFfAuto->isChecked());
}

void OptionsDialog::hideEvent(QHideEvent* e) {
    QDialog::hideEvent(e);
    enable_ffmpeg_check_timer(false);
}

void OptionsDialog::set_config(const PersistentConfig& config) {
    m_config = config;

    if (config.path_config.use_system_ffmpeg) {
        ui->rbFfAuto->setChecked(true);
    } else {
        ui->rbFfManual->setChecked(true);
    }

    ui->pcFfmpegPath->set_current_path(config.path_config.ffmpeg_path);
    ui->pcSoundfontPath->set_current_path(config.path_config.soundfont_path);

    ui->cmbVideoCodec->setCurrentIndex(static_cast<int>(config.video_config.codec));
    ui->cmbEncodeSpeed->setCurrentIndex(static_cast<int>(config.video_config.preset));
    ui->sbCrf->setValue(config.video_config.crf);

    ui->sbAudioBitrate->setValue(config.audio_config.bitrate_kbps);
}

PersistentConfig OptionsDialog::get_config() {
    return m_config;
}

void OptionsDialog::update_config() {
    m_config.path_config.use_system_ffmpeg = ui->rbFfAuto->isChecked();
    m_config.path_config.ffmpeg_path = ui->pcFfmpegPath->current_path();
    m_config.path_config.soundfont_path = ui->pcSoundfontPath->current_path();

    m_config.video_config.codec = static_cast<VideoCodec>(
        ui->cmbVideoCodec->currentIndex());
    m_config.video_config.preset = static_cast<H26xPreset>(
        ui->cmbEncodeSpeed->currentIndex());
    m_config.video_config.crf = ui->sbCrf->value();

    m_config.audio_config.bitrate_kbps = ui->sbAudioBitrate->value();
}

void OptionsDialog::enable_ffmpeg_check_timer(bool enable) {
    if (enable) {
        m_timer.start();
        check_path_for_ffmpeg();
    } else {
        ui->lbFfmpegFound->hide();
        ui->lbFfmpegNotFound->hide();
        m_timer.stop();
    }
}

void OptionsDialog::check_path_for_ffmpeg() {
    QString ffmpeg_path = search_path("ffmpeg");
    if (ffmpeg_path.isNull()) {
        ui->lbFfmpegFound->hide();
        ui->lbFfmpegNotFound->show();
    } else {
        ui->lbFfmpegFound->setText(QString("FFmpeg found at %1").arg(ffmpeg_path));
        ui->lbFfmpegNotFound->hide();
        ui->lbFfmpegFound->show();
    }
}

void OptionsDialog::reset_crf_to_default() {
    VideoCodec codec = static_cast<VideoCodec>(ui->cmbVideoCodec->currentIndex());
    switch (codec) {
    case VideoCodec::H264:
        ui->sbCrf->setValue(23);
        break;
    case VideoCodec::H265:
        ui->sbCrf->setValue(28);
        break;
    case VideoCodec::Invalid:
        break;
    }
}

#ifdef Q_OS_WINDOWS

QString OptionsDialog::search_path(const QString& exe) {
    const auto paths = m_env.value("PATH").split(';');
    const auto pathexts = m_env.value("PATHEXT").toCaseFolded().split(';');

    QStringList name_filters;
    for (const auto& ext : pathexts) {
        name_filters << exe + ext;
    }

    for (const auto& path : paths) {
        if (path.isEmpty())
            continue;

        int last_priority = std::numeric_limits<int>::max();
        QString best_candidate;
        for (const auto& dirent :
             QDirListing(path, name_filters, QDirListing::IteratorFlag::FilesOnly)) {
            auto suffix = "." + dirent.suffix().toCaseFolded();
            int priority = pathexts.indexOf(suffix);

            if (priority == -1) // Should never happen; for completeness
                continue;

            if (priority == 0)
                return dirent.canonicalFilePath();

            if (priority < last_priority) {
                last_priority = priority;
                best_candidate = dirent.canonicalFilePath();
            }
        }

        if (!best_candidate.isEmpty())
            return best_candidate;
    }

    return QString();
}

#else

QString OptionsDialog::search_path(const QString& exe) {
    const auto paths = m_env.value("PATH").split(':');

    auto flags = QDirListing::IteratorFlag::FilesOnly
                 | QDirListing::IteratorFlag::CaseSensitive;
    QStringList filter{exe};
    for (const auto& path : paths) {
        if (path.isEmpty())
            continue;

        for (const auto& dirent : QDirListing(path, {exe}, flags)) {
            return dirent.canonicalFilePath();
        }
    }

    return QString();
}

#endif
