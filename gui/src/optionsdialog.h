#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QHideEvent>
#include <QProcessEnvironment>
#include <QTimer>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();

    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

    QString ffmpeg_path() const;
    void set_ffmpeg_path(const QString& path);
    bool use_system_ffmpeg() const;
    void set_use_system_ffmpeg(bool);
    QString soundfont_path() const;
    void set_soundfont_path(const QString& path);

public slots:
    void enable_ffmpeg_check_timer(bool);
    void check_path_for_ffmpeg();

private:
    Ui::OptionsDialog* ui;

    QTimer m_timer;
    QProcessEnvironment m_env;

    QString search_path(const QString& exe);
};

#endif // OPTIONSDIALOG_H
