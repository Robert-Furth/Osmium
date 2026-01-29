#include "workers.h"
#include "config.h"

#include <chrono>
#include <format>
#include <random>

#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMutexLocker>
#include <QObject>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QString>
#include <QSurfaceFormat>

// -- AbstractSocketWorker --

AbstractSocketWorker::AbstractSocketWorker(const QString& prefix, QObject* parent)
    : QObject(parent),
      m_server(new QLocalServer(this)),
      m_accept_new_connections(false) {
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

    if (m_accept_new_connections) {
        m_accept_new_connections = false;
        m_abort_requested = false;
        handle_connection(connection);
    }

    connection->disconnectFromServer();
}

// -- VideoSocketWorker --

const int MSAA_SAMPLES = 4;

VideoSocketWorker::VideoSocketWorker()
    : AbstractSocketWorker("osvid-"),
      m_offscreen_surface(new QOffscreenSurface(nullptr, this)),
      m_opengl_ctx(new QOpenGLContext(this)) {

    // Enable multisampling for AA
    QSurfaceFormat surface_format;
    surface_format.setSamples(MSAA_SAMPLES);

    m_opengl_ctx->setFormat(surface_format);
    m_opengl_ctx->create();
    m_offscreen_surface->setFormat(m_opengl_ctx->format());
    m_offscreen_surface->create();
}

VideoSocketWorker::~VideoSocketWorker() {
    m_offscreen_surface->destroy();
}

void VideoSocketWorker::init(const QString& filename,
                             const QString& soundfont,
                             const QList<ChannelArgs>& channel_args,
                             const GlobalArgs& global_args) {
    m_width = global_args.width;
    m_height = global_args.height;
    m_fps = global_args.fps;

    try {
        m_renderer.emplace(filename, soundfont, channel_args, global_args);
    } catch (const std::runtime_error& e) {
        m_renderer.reset();
        throw;
    }

    m_accept_new_connections = true;
}

void VideoSocketWorker::handle_connection(QLocalSocket* connection) {
    if (!m_renderer) {
        emit done(false, "Renderer not initialized");
        return;
    }

    if (!m_opengl_ctx->makeCurrent(m_offscreen_surface)) {
        m_renderer.reset();
        emit done(false, "Could not make OpenGL context current");
        return;
    }

    QOpenGLFramebufferObjectFormat framebuffer_format;
    framebuffer_format.setAttachment(
        QOpenGLFramebufferObject::Attachment::CombinedDepthStencil);
    framebuffer_format.setSamples(MSAA_SAMPLES);
    framebuffer_format.setInternalTextureFormat(GL_RGB);

    QOpenGLFramebufferObject framebuffer(m_width, m_height, framebuffer_format);

    QString status_str;
    bool ok = handle_connection_inner(connection, framebuffer, status_str);

    connection->flush();
    m_opengl_ctx->doneCurrent();
    m_renderer.reset();
    emit done(ok, status_str);
}

bool VideoSocketWorker::handle_connection_inner(QLocalSocket* connection,
                                                QOpenGLFramebufferObject& framebuffer,
                                                QString& out_status_str) {

    using ms = std::chrono::milliseconds;
    using clock = std::chrono::steady_clock;
    using std::chrono::duration_cast;

    QOpenGLPaintDevice paint_device(m_width, m_height);

    int frame_counter = 0;
    int preview_update_freq = std::max(1, m_fps / 2);
    ms total_frameadvance_ms(0);
    ms total_paint_ms(0);
    ms total_toimg_ms(0);
    ms total_write_ms(0);

    auto render_start = clock::now();
    while (m_renderer->has_frames_remaining() && !m_abort_requested) {
        if (!framebuffer.bind()) {
            out_status_str = "Could not bind OpenGL framebufffer";
            return false;
        }

        QPainter painter(&paint_device);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

        auto frameadvance_start = clock::now();
        m_renderer->advance_frame();
        total_frameadvance_ms += duration_cast<ms>(clock::now() - frameadvance_start);

        auto paint_start = clock::now();
        // auto frame = m_renderer->paint_next_frame();
        m_renderer->paint(painter);
        total_paint_ms += duration_cast<ms>(clock::now() - paint_start);

        auto toimage_start = clock::now();
        // TODO: toImage() is fairly inefficient (takes up the bulk of render time).
        // Any good alternatives? Or is this just a consequence of transferring data
        // between the GPU and CPU?
        auto frame = framebuffer.toImage();
        total_toimg_ms += duration_cast<ms>(clock::now() - toimage_start);

        auto write_start = clock::now();
        auto write_result =
            connection->write(reinterpret_cast<const char*>(frame.constBits()),
                              frame.sizeInBytes());
        total_write_ms += duration_cast<ms>(clock::now() - write_start);

        if (write_result == -1) {
            out_status_str =
                QString("Error writing frame data: %1").arg(connection->errorString());
            return false;
        }

        if (frame_counter % preview_update_freq == 0) {
            emit preview_image_changed(QPixmap::fromImage(frame));
        }
        emit progress_changed(m_renderer->get_progress() * 1000);

        frame_counter += 1;
    }

    auto render_dur = duration_cast<ms>(clock::now() - render_start);
    double framecount_double = static_cast<double>(frame_counter);

    qDebug() << "VIDEO:" << frame_counter << "frames";
    qDebug() << "Average frame advance time:"
             << total_frameadvance_ms.count() / framecount_double << "ms";
    qDebug() << "Average frame paint time:" << total_paint_ms.count() / framecount_double
             << "ms";
    qDebug() << "Average frame toImage() time:"
             << total_toimg_ms.count() / framecount_double << "ms";
    qDebug() << "Average socket write time:" << total_write_ms.count() / framecount_double
             << "ms";
    qDebug() << "Total render time:" << std::format("{:%M:%S}", render_dur).c_str();

    if (m_abort_requested) {
        out_status_str = "Rendering aborted";
    }
    return true;
}

// -- AudioSocketWorker --

AudioSocketWorker::AudioSocketWorker() : AbstractSocketWorker("osaud-") {}

uint32_t AudioSocketWorker::get_num_channels() {
    if (!m_player.has_value())
        return 0;
    return m_player->get_num_channels();
}

void AudioSocketWorker::init(const QString& filename, const QString& soundfont, int fps) {
    m_filename = filename;
    try {
        m_player.emplace(m_filename.toUtf8(), fps, soundfont.toUtf8());
    } catch (const std::runtime_error& e) {
        m_player.reset();
        throw;
    }

    m_accept_new_connections = true;
}

void AudioSocketWorker::handle_connection(QLocalSocket* connection) {
    if (!m_player) {
        emit done(false, "Player not initialized");
        return;
    }

    int frame_no = 0;
    while (m_player->is_playing() && !m_abort_requested) {
        m_player->next_wave_data();
        const auto& samples = m_player->get_samples();
        auto write_result =
            connection->write(reinterpret_cast<const char*>(samples.data()),
                              samples.size() * sizeof(float));
        if (write_result == -1) {
            auto err_message =
                QString("Error writing audio data: %1").arg(connection->errorString());
            m_player.reset();
            emit done(false, err_message);
            return;
        }
        frame_no++;
    }

    qDebug() << "AUDIO:" << frame_no << "frames";
    connection->flush();
    m_player.reset();
    emit done(true, m_abort_requested ? "Rendering aborted" : "");
}

// -- RenderWorker --

RenderWorker::RenderWorker(QObject* parent)
    : QObject(parent),
      m_ffmpeg(this),
      m_video_thread(this),
      m_audio_thread(this),
      m_state(State::Idle),
      m_status(true),
      m_status_message("") {
    m_vs_worker = new VideoSocketWorker();
    m_vs_worker->moveToThread(&m_video_thread);
    m_video_server_path = m_vs_worker->get_full_path();

    connect(m_vs_worker,
            &VideoSocketWorker::done,
            this,
            &RenderWorker::notify_child_worker_done);
    m_video_thread.start();

    m_as_worker = new AudioSocketWorker();
    m_as_worker->moveToThread(&m_audio_thread);
    m_audio_server_path = m_as_worker->get_full_path();

    connect(m_as_worker,
            &AudioSocketWorker::done,
            this,
            &RenderWorker::notify_child_worker_done);
    m_audio_thread.start();

    connect(&m_ffmpeg, &QProcess::finished, this, &RenderWorker::notify_ffmpeg_done);
    connect(&m_ffmpeg,
            &QProcess::errorOccurred,
            this,
            &RenderWorker::notify_ffmpeg_error);
}

void RenderWorker::work(const QString& input_file,
                        const QString& soundfont,
                        const QString& ffmpeg_path,
                        const QString& output_file,
                        const QList<ChannelArgs>& channel_args,
                        const GlobalArgs& global_args) {
    QMutexLocker lock(&m_state_mutex);
    if (m_state != State::Idle)
        return;

    m_state = State::Initializing;
    m_status = true;
    m_status_message = "";
    m_global_args = global_args;
    m_output_path = output_file;
    m_ffmpeg_path = ffmpeg_path;

    try {
        if (m_ffmpeg.state() == QProcess::Running) {
            lock.unlock();
            m_ffmpeg.kill();
            m_ffmpeg.waitForFinished();
            lock.relock();
        }

        m_vs_worker->init(input_file, soundfont, channel_args, global_args);
        m_as_worker->init(input_file, soundfont, global_args.fps);

        if (ffmpeg_path.isNull()) {
            m_ffmpeg.setProgram("ffmpeg");
        } else {
            m_ffmpeg.setProgram(ffmpeg_path);
        }
        m_ffmpeg.setArguments(get_ffmpeg_args());

        qDebug() << "Running ffmpeg with args:" << m_ffmpeg.arguments();
        m_state = State::Running;
        lock.unlock();
        m_ffmpeg.start();
    } catch (const std::exception& e) {
        emit done(false, QString(e.what()).toHtmlEscaped());
        m_state = State::Idle;
        return;
    }
}

void RenderWorker::request_stop() {
    m_vs_worker->request_stop();
    m_as_worker->request_stop();
    if (m_status_message.isEmpty()) {
        m_status_message = "Rendering aborted.";
    }
    m_ffmpeg.terminate();
}

QStringList RenderWorker::get_ffmpeg_args() {
    int fps = m_global_args.fps;
    int width = m_global_args.width;
    int height = m_global_args.height;
    double vol = m_global_args.volume;

    QString vid_preset = to_string(m_global_args.h26x_preset);
    int crf = m_global_args.crf;
    int bitrate = m_global_args.bitrate_kbps;

    QString vid_codec;
    switch (m_global_args.vid_codec) {
    case VideoCodec::H264:
        vid_codec = "libx264";
        break;
    case VideoCodec::H265:
        vid_codec = "libx265";
        break;
    }

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
                         << "-c:v" << vid_codec << "-crf" << QString::number(crf)
                         << "-preset" << vid_preset << "-filter:v"
                         << "format=yuv420p"
                         // output audio format
                         << "-c:a" << "aac" << "-b:a" << QString("%1k").arg(bitrate)
                         << "-filter:a"
                         << QString("volume=%1").arg(vol)
                         // output file
                         << m_output_path;
}

void RenderWorker::notify_child_worker_done(bool ok, const QString& message) {
    QMutexLocker lock(&m_state_mutex);
    if (m_state != State::Running)
        return;

    m_status = m_status && ok;
    if (m_status_message.isEmpty() && !message.isEmpty()) {
        m_status_message = message;
    }

    if (!m_status) {
        request_stop();
    }
}

void RenderWorker::notify_ffmpeg_done(int status_code) {
    QMutexLocker lock(&m_state_mutex);
    qDebug() << "ffmpeg exited with status code" << status_code;
    if (m_state == State::Initializing) {
        qDebug() << "(finished while initializing)";
        return;
    }

    if (m_state == State::Running) {
        if (m_vs_worker)
            m_vs_worker->request_stop();
        if (m_as_worker)
            m_as_worker->request_stop();
    }

    if (status_code != 0) {
        m_status = false;
        if (m_status_message.isEmpty()) {
            m_status_message =
                QString("FFmpeg exited abnormally (status code %1).").arg(status_code);
        }
    }

    emit done(m_status, m_status_message.toHtmlEscaped());
    m_state = State::Idle;
}

void RenderWorker::notify_ffmpeg_error(QProcess::ProcessError err) {
    if (err == QProcess::ProcessError::FailedToStart) {
        QMutexLocker lock(&m_state_mutex);

        QString message;
        if (m_ffmpeg_path.isNull()) {
            message = "Could not start FFmpeg; it was not found in the system path. If "
                      "you've installed FFmpeg already, either specify it manually in "
                      "the Program Options menu or add it to the system path. If you "
                      "haven't installed FFmpeg yet, you can download it from <a "
                      "href='https://ffmpeg.org/download.html'>its website</a>.";
        } else {
            message =
                QString(
                    "Could not start FFmpeg; the file \"%1\" either does not exist or "
                    "is not executable. Specify the proper path in the Program Options "
                    "menu. If you haven't installed FFmpeg yet, you can download it "
                    "from <a href='https://ffmpeg.org/download.html'>its website</a>.")
                    .arg(m_ffmpeg_path.toHtmlEscaped());
        }

        emit done(false, message);
        m_state = State::Idle;
    }
}
