#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QHideEvent>
#include <QProcessEnvironment>
#include <QTimer>

#include "config.h"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget* parent = nullptr);
    ~OptionsDialog();

    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;

    void set_config(const PersistentConfig& config);
    PersistentConfig get_config();
    void update_config();

public slots:
    void enable_ffmpeg_check_timer(bool);
    void check_path_for_ffmpeg();

    void reset_crf_to_default();

private:
    Ui::OptionsDialog* ui;

    QTimer m_timer;
    QProcessEnvironment m_env;
    PersistentConfig m_config;

    QString search_path(const QString& exe);
};

#endif // OPTIONSDIALOG_H
