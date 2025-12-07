#include "colorpicker.h"
#include "ui_colorpicker.h"

#include <QColorDialog>
#include <QPalette>

namespace controls {

ColorPicker::ColorPicker(QWidget* parent) : QWidget(parent), ui(new Ui::ColorPicker) {
    ui->setupUi(this);

    setFocusProxy(ui->btnChoose);

    auto palette = ui->lblColor->palette();
    palette.setColor(QPalette::Window, m_color);
    ui->lblColor->setPalette(palette);
}

ColorPicker::~ColorPicker() {
    delete ui;
}

void ColorPicker::setColor(const QColor& color) {
    m_color = color;

    auto palette = ui->lblColor->palette();
    palette.setColor(QPalette::Window, m_color);
    ui->lblColor->setPalette(palette);

    emit colorChanged(m_color);
}

void ColorPicker::setAllowAlpha(bool allow) {
    m_allow_alpha = allow;
    emit allowAlphaChanged(allow);
}

void ColorPicker::openPicker() {
    QColorDialog::ColorDialogOptions opts;
    opts = opts.setFlag(QColorDialog::ShowAlphaChannel, m_allow_alpha);

    auto color = QColorDialog::getColor(m_color, this, "", opts);
    if (color.isValid()) {
        setColor(color);
    }
}

} // namespace controls
