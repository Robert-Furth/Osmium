#include "colorpicker.h"

#include <QColorDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedLayout>

namespace controls {

// clang-format off
const uchar CHECKERBOARD[] = {
    160,160,160,255, 255,255,255,255, 160,160,160,255, 255,255,255,255,
    255,255,255,255, 160,160,160,255, 255,255,255,255, 160,160,160,255,
    160,160,160,255, 255,255,255,255, 160,160,160,255, 255,255,255,255,
    255,255,255,255, 160,160,160,255, 255,255,255,255, 160,160,160,255,
};
// clang-format on

ColorPicker::ColorPicker(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    setLayout(layout);

    auto* frame = new QFrame();
    frame->setFixedSize(24, 24);
    frame->setFrameShape(QFrame::Shape::StyledPanel);
    layout->addWidget(frame);

    auto* stack_layout = new QStackedLayout();
    stack_layout->setStackingMode(QStackedLayout::StackingMode::StackAll);
    stack_layout->setContentsMargins(0, 0, 0, 0);
    frame->setLayout(stack_layout);

    m_lbl_color = new ClickableLabel();
    m_lbl_color->setAutoFillBackground(true);
    stack_layout->addWidget(m_lbl_color);
    connect(m_lbl_color, &ClickableLabel::clicked, this, &ColorPicker::openPicker);

    auto* lbl_checkerboard = new QLabel();
    lbl_checkerboard->setScaledContents(true);

    lbl_checkerboard->setPixmap(get_checkerboard_pixmap());
    stack_layout->addWidget(lbl_checkerboard);

    auto* btn = new QPushButton("Choose...");
    setFocusProxy(btn);
    connect(btn, &QPushButton::clicked, this, &ColorPicker::openPicker);
    layout->addWidget(btn);

    // ui->stackedWidget->layout()
    auto palette = m_lbl_color->palette();
    palette.setColor(QPalette::Window, m_color);
    m_lbl_color->setPalette(palette);
}

void ColorPicker::setColor(const QColor& color) {
    m_color = color;

    auto palette = m_lbl_color->palette();
    palette.setColor(QPalette::Window, m_color);
    m_lbl_color->setPalette(palette);

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

std::optional<QPixmap> ColorPicker::s_checkerboard_pixmap = std::nullopt;
QPixmap& ColorPicker::get_checkerboard_pixmap() {
    if (!s_checkerboard_pixmap) {
        s_checkerboard_pixmap =
            QPixmap::fromImage(QImage(CHECKERBOARD, 4, 4, QImage::Format_RGBX8888)
                                   .scaled(24, 24)
                                   .copy(1, 1, 22, 22));
    }

    return *s_checkerboard_pixmap;
}

} // namespace controls
