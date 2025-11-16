#ifndef WORKERS_H
#define WORKERS_H

#include <atomic>
#include <optional>

#include <QLocalServer>
#include <QLocalSocket>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QProcess>
#include <QString>
#include <QThread>

#include <osmium.h>

#include "scoperenderer.h"

class AbstractSocketWorker : public QObject {
    Q_OBJECT
public:
    AbstractSocketWorker(const QString& prefix = "", QObject* parent = nullptr);
    QString get_full_path();

public slots:
    void request_stop() { m_abort_requested = true; }

protected:
    QLocalServer* m_server;
    bool m_accept_new_connections;
    std::atomic<bool> m_abort_requested;

    virtual void handle_connection(QLocalSocket*) = 0;

private slots:
    void priv_on_connection();
};

class VideoSocketWorker : public AbstractSocketWorker {
    Q_OBJECT
public:
    VideoSocketWorker();

public slots:
    void init(const QString& filename,
              const QString& soundfont,
              const QList<ChannelArgs>& channel_args,
              const GlobalArgs& global_args);

signals:
    void ready();
    void init_error(const QString& msg);
    void done(bool, const QString& msg = "");
    void progress_changed(int);
    void preview_image_changed(const QPixmap&);

protected:
    void handle_connection(QLocalSocket*) override;

private:
    int m_width = 0;
    int m_height = 0;
    int m_fps = 0;

    std::optional<ScopeRenderer> m_renderer;
};

class AudioSocketWorker : public AbstractSocketWorker {
    Q_OBJECT
public:
    AudioSocketWorker();

    uint32_t get_num_channels();

public slots:
    void init(const QString& filename, const QString& soundfont, int fps);

signals:
    void ready();
    void init_error(const QString& msg);
    void done(bool, const QString& msg = "");

protected:
    void handle_connection(QLocalSocket*) override;

private:
    QString m_filename;
    std::optional<osmium::Player> m_player;
};

class RenderWorker : public QObject {
    Q_OBJECT

    enum State { Idle, Initializing, Running };

public:
    RenderWorker(QObject* parent = nullptr);

    const VideoSocketWorker* video_worker() { return m_vs_worker; }
    const AudioSocketWorker* audio_worker() { return m_as_worker; }

public slots:
    void work(const QString& input_file,
              const QString& soundfont,
              const QString& ffmpeg_path,
              const QString& output_file,
              const QList<ChannelArgs>& channel_args,
              const GlobalArgs& global_args);
    void request_stop();

signals:
    void init_video(const QString& input_file,
                    const QString& soundfont,
                    const QList<ChannelArgs>& channel_args,
                    const GlobalArgs& global_args);
    void init_audio(const QString& filename, const QString& soundfont, int fps);
    void error(const QString& msg);
    void progress_changed(int);
    void done(bool, const QString& msg);

private:
    QProcess m_ffmpeg;
    VideoSocketWorker* m_vs_worker;
    AudioSocketWorker* m_as_worker;
    QThread m_video_thread;
    QThread m_audio_thread;
    QString m_video_server_path;
    QString m_audio_server_path;

    GlobalArgs m_global_args;
    QString m_output_path;
    QString m_ffmpeg_path;

    QMutex m_state_mutex;
    State m_state;
    bool m_status;
    QString m_status_message;

    // void start_rendering();
    QStringList get_ffmpeg_args();

private slots:
    void notify_child_worker_done(bool, const QString&);
    void notify_ffmpeg_done(int);
    void notify_ffmpeg_error(QProcess::ProcessError);
};

#endif // WORKERS_H
