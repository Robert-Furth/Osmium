#include "pathchooser.h"

#include <QFileDialog>
#include <QHBoxLayout>

namespace controls {

PathChooser::PathChooser(QWidget* parent) : QWidget(parent) {
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_line_edit = new QLineEdit(this);
    m_line_edit->setReadOnly(true);
    layout->addWidget(m_line_edit);

    m_choose_button = new QPushButton("Choose...", this);
    layout->addWidget(m_choose_button);

    connect(m_choose_button, &QPushButton::clicked, this, &PathChooser::open_file_chooser);

    setLayout(layout);
}

void PathChooser::set_current_path(const QString& s) {
    if (s == m_current_path)
        return;

    m_current_path = s;
    m_line_edit->setText(s);
    emit path_changed(s);
}

void PathChooser::set_icon(const QIcon& icon) {
    m_choose_button->setIcon(icon);
    m_icon = icon;
}

void PathChooser::open_file_chooser() {
    QString initial_dir = m_initial_dir;
    if (initial_dir.isEmpty() && !m_current_path.isEmpty()) {
        initial_dir = QFileInfo(m_current_path).absoluteDir().path();
    }

    QString filename = QFileDialog::getOpenFileName(this,
                                                    m_dialog_title,
                                                    initial_dir,
                                                    m_filter);
    if (filename.isNull())
        return;

    set_current_path(filename);
}

} // namespace controls
