#include "workers.h"

#include <chrono>
#include <format>
#include <random>

#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QString>

#include "constants.h"

// -- AbstractSocketWorker --

AbstractSocketWorker::AbstractSocketWorker(const QString& prefix, QObject* parent)
    : QObject(parent),
      m_server(new QLocalServer(this)),
      m_is_ready(false) {
    connect(m_server,
            &QLocalServer::newConnection,
            this,
            &AbstractSocketWorker::priv_on_connection);

    // Generate a random name for the local server
    std::random_device rd;
    std::default_random_engine rand(rd());
    QString name(prefix);
    for (int i = 0; i < 20; i++) {
        char c = 'a' + (rand() % 26);
        name.append(c);
    }

    // Start listening
    m_server->listen(name);
}

QString AbstractSocketWorker::get_full_path() {
    return m_server->fullServerName();
}

void AbstractSocketWorker::priv_on_connection() {
    QLocalSocket* connection = m_server->nextPendingConnection();
    connect(connection,
            &QLocalSocket::disconnected,
            connection,
            &QLocalSocket::deleteLater);

    if (m_is_ready) {
        m_is_ready = false;
        m_abort_requested = false;
        handle_connection(connection);
    }

    // if (connection->state() == QLocalSocket::ConnectedState) {
    connection->disconnectFromServer();
    // }
}

// -- VideoSocketWorker --

VideoSocketWorker::VideoSocketWorker() : AbstractSocketWorker("osvid-") {}

void VideoSocketWorker::init(const QStringList& filenames,
                             const QList<ScopeRenderer::ChannelArgs> channel_args,
                             const ScopeRenderer::GlobalArgs global_args) {
    m_width = global_args.width;
    m_height = global_args.height;
    m_fps = global_args.fps;

    m_renderer.emplace(filenames[0], channel_args, global_args);
    m_is_ready = true;
    emit ready();
}

void VideoSocketWorker::handle_connection(QLocalSocket* connection) {
    // QImage img(m_width, m_height, QImage::Format_RGB32);
    // QPixmap pixmap(m_width, m_height);
    // if (img.isNull()) {
    //     emit done(false, "Could not allocate a framebuffer -- out of memory?");
    //     return;
    // }

    // QPainter painter(&img);
    // painter.setRenderHint(QPainter::Antialiasing);

    typedef std::chrono::milliseconds ms;

    int frame_counter = 0;
    int preview_update_freq = std::max(1, m_fps / 4);
    unsigned long long elapsed_ms = 0;
    std::chrono::steady_clock clock;
    auto render_start = clock.now();
    while (m_renderer->has_frames_remaining() && !m_abort_requested) {
        auto frame_start = clock.now();
        auto frame = m_renderer->paint_frame();
        // pic.play(&painter);
        auto frame_dur = std::chrono::duration_cast<ms>(clock.now() - frame_start);
        elapsed_ms += frame_dur.count();

        connection->flush();
        // QImage frame = pixmap.toImage().convertToFormat(QImage::Format_RGB32);
        qint64 write_result = connection->write(reinterpret_cast<const char*>(
                                                    frame.constBits()),
                                                frame.sizeInBytes());
        if (write_result == -1) {
            auto err_message = QString("Error writing frame data: %1")
                                   .arg(connection->errorString());
            emit done(false, err_message);
            return;
        }

        if (frame_counter % preview_update_freq == 0) {
            emit preview_image_changed(QPixmap::fromImage(frame));
        }
        emit progress_changed(m_renderer->get_progress() * 1000);
        // qDebug() << "VIDEO" << frame_counter;

        frame_counter += 1;
    }

    auto render_dur = std::chrono::duration_cast<ms>(clock.now() - render_start);

    qDebug() << "Average frame render time:"
             << elapsed_ms / static_cast<double>(frame_counter) << "ms";
    qDebug() << "Total render time:" << std::format("{:%M:%S}", render_dur).c_str();

    connection->flush();
    m_renderer.reset();
    emit done(true);
}

// -- AudioSocketWorker --

AudioSocketWorker::AudioSocketWorker() : AbstractSocketWorker("osaud-") {}

uint32_t AudioSocketWorker::get_num_channels() {
    if (!m_player.has_value())
        return 0;
    return m_player->get_num_channels();
}

void AudioSocketWorker::init(const QString& filename, int fps) {
    m_filename = filename;
    m_player.emplace(m_filename.toUtf8(), fps);
    m_is_ready = true;
    emit ready();
}

void AudioSocketWorker::handle_connection(QLocalSocket* connection) {
    while (m_player->is_playing() && !m_abort_requested) {
        m_player->next_wave_data();
        auto& samples = m_player->get_samples();
        qint64 write_result = connection->write(reinterpret_cast<const char*>(
                                                    samples.data()),
                                                samples.size() * sizeof(float));
        if (write_result == -1) {
            auto err_message = QString("Error writing audio data: %1")
                                   .arg(connection->errorString());
            emit done(false, err_message);
            return;
        }
    }

    connection->flush();
    emit done(true);
}

// -- RenderWorker --

RenderWorker::RenderWorker(QObject* parent)
    : QObject(parent),
      m_ffmpeg(this),
      m_video_thread(this),
      m_audio_thread(this),
      m_is_running(false),
      m_audio_is_ready(false),
      m_video_is_ready(false),
      m_video_is_done(false),
      m_audio_is_done(false) {
    m_vs_worker = new VideoSocketWorker();
    m_vs_worker->moveToThread(&m_video_thread);
    m_video_server_path = m_vs_worker->get_full_path();
    connect(this, &RenderWorker::init_video, m_vs_worker, &VideoSocketWorker::init);
    connect(m_vs_worker, &VideoSocketWorker::ready, this, &RenderWorker::set_video_ready);
    connect(m_vs_worker, &VideoSocketWorker::done, this, &RenderWorker::notify_video_done);
    m_video_thread.start();

    m_as_worker = new AudioSocketWorker();
    m_as_worker->moveToThread(&m_audio_thread);
    m_audio_server_path = m_as_worker->get_full_path();
    connect(this, &RenderWorker::init_audio, m_as_worker, &AudioSocketWorker::init);
    connect(m_as_worker, &AudioSocketWorker::ready, this, &RenderWorker::set_audio_ready);
    connect(m_as_worker, &AudioSocketWorker::done, this, &RenderWorker::notify_audio_done);
    m_audio_thread.start();
}

void RenderWorker::work(const QStringList& filenames,
                        const QList<ScopeRenderer::ChannelArgs> channel_args,
                        const ScopeRenderer::GlobalArgs global_args) {
    if (m_is_running)
        return;

    m_is_running = true;
    m_video_is_ready = false;
    m_video_is_done = false;
    m_audio_is_ready = false;
    m_audio_is_done = false;
    m_global_args = global_args;

    emit init_video(filenames, channel_args, global_args);
    emit init_audio(filenames[0], global_args.fps);
}

void RenderWorker::request_stop() {
    m_vs_worker->request_stop();
    m_as_worker->request_stop();
    m_ffmpeg.terminate();
}

void RenderWorker::start_rendering() {
    if (m_ffmpeg.state() == QProcess::Running) {
        m_ffmpeg.kill();
        m_ffmpeg.waitForFinished();
    }
    m_ffmpeg.setProgram("ffmpeg");
    m_ffmpeg.setArguments(get_ffmpeg_args());
    m_ffmpeg.start();
}

QStringList RenderWorker::get_ffmpeg_args() {
    int fps = m_global_args.fps;
    int width = m_global_args.width;
    int height = m_global_args.height;
    return QStringList() << "-y"
                         // input video format
                         << "-f" << "rawvideo" << "-pixel_format" << "rgb32"
                         << "-framerate" << QString::number(fps) << "-video_size"
                         << QString("%1x%2").arg(width).arg(height) << "-i"
                         << m_video_server_path
                         // input audio format
                         << "-f" << "f32le" << "-sample_rate" << QString::number(48000)
                         << "-ac" << QString::number(m_as_worker->get_num_channels())
                         << "-i"
                         << m_audio_server_path
                         // output video format
                         << "-filter:v" << "format=pix_fmts=yuv420p" << "-c:v"
                         << "libx264"
                         // output audio format
                         << "-c:a" << "aac" << "-b:a"
                         << "192k"
                         // output file
                         << DEBUG_OUTPUT_PATH;
}

void RenderWorker::set_video_ready() {
    m_video_is_ready = true;
    if (m_audio_is_ready) {
        start_rendering();
    }
}

void RenderWorker::set_audio_ready() {
    m_audio_is_ready = true;
    if (m_video_is_ready) {
        start_rendering();
    }
}

void RenderWorker::notify_video_done(bool ok, const QString& message) {
    // m_video_thread.quit();
    m_video_is_done = true;

    if (ok && m_audio_is_done) {
        m_ffmpeg.waitForFinished();
        emit done(true, "");
    } else if (!ok && !m_audio_is_done) {
        emit done(false, message);
        request_stop();
    }

    m_is_running = !m_video_is_done || !m_audio_is_done;
}

void RenderWorker::notify_audio_done(bool ok, const QString& message) {
    // m_audio_thread.quit();
    m_audio_is_done = true;

    if (ok && m_video_is_done) {
        m_ffmpeg.waitForFinished();
        emit done(true, "");
    } else if (!ok && !m_video_is_done) {
        emit done(false, message);
        request_stop();
    }

    m_is_running = !m_video_is_done || !m_audio_is_done;
}
