#ifndef PATHCHOOSER_H
#define PATHCHOOSER_H

#include <QIcon>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace controls {

class PathChooser : public QWidget {
    Q_OBJECT

    Q_PROPERTY(QString filter READ filter WRITE set_filter)
    Q_PROPERTY(QIcon icon READ icon WRITE set_icon)
    Q_PROPERTY(QString dialog_title READ dialog_title WRITE set_dialog_title)

public:
    explicit PathChooser(QWidget* parent = nullptr);

    QString current_path() const { return m_current_path; }
    void set_current_path(const QString& s);

    QString dialog_title() const { return m_dialog_title; }
    void set_dialog_title(const QString& s) { m_dialog_title = s; }

    QString filter() const { return m_filter; }
    void set_filter(const QString& filter) { m_filter = filter; }

    QString initial_dir() const { return m_initial_dir; }
    void set_initial_dir(const QString& dir) { m_initial_dir = dir; }

    QIcon icon() const { return m_icon; }
    void set_icon(const QIcon& icon);

public slots:
    void open_file_chooser();

signals:
    void path_changed(const QString& path);

private:
    QLineEdit* m_line_edit;
    QPushButton* m_choose_button;

    QString m_current_path = "";
    QString m_dialog_title = "Choose File";
    QString m_filter = "";
    QString m_initial_dir = "";
    QIcon m_icon;
};

} // namespace controls

#endif // PATHCHOOSER_H
