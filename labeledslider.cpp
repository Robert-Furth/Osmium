#include "labeledslider.h"

#include <QHBoxLayout>
#include <QLocale>

namespace controls {

LabeledSlider::LabeledSlider(QWidget* parent)
    : QAbstractSlider(parent),
      m_label_format("%v") {
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_slider = new QSlider();
    m_slider->setOrientation(Qt::Horizontal);
    layout->addWidget(m_slider);
    connect(m_slider, &QSlider::valueChanged, this, &LabeledSlider::setValue);
    connect(this, &LabeledSlider::rangeChanged, m_slider, &QSlider::setRange);
    connect(this, &LabeledSlider::rangeChanged, this, &LabeledSlider::update_label_size);
    connect(this,
            &LabeledSlider::labelFormatChanged,
            this,
            &LabeledSlider::update_label_size);

    m_label = new QLabel();
    layout->addWidget(m_label);

    setLayout(layout);
}

QString LabeledSlider::labelFormat() const {
    return m_label_format;
}

void LabeledSlider::setLabelFormat(const QString& format) {
    m_label_format = format;
    emit labelFormatChanged(format);
    update_label();
}

Qt::Alignment LabeledSlider::labelAlignment() const {
    return m_label->alignment();
}

void LabeledSlider::setLabelAlignment(Qt::Alignment alignment) {
    if (alignment == labelAlignment())
        return;

    m_label->setAlignment(alignment);
    emit labelAlignmentChanged(alignment);
}

QSlider::TickPosition LabeledSlider::tickPosition() const {
    return m_slider->tickPosition();
}

void LabeledSlider::setTickPosition(QSlider::TickPosition tpos) {
    m_slider->setTickPosition(tpos);
}

int LabeledSlider::tickInterval() const {
    return m_slider->tickInterval();
}

void LabeledSlider::setTickInterval(int interval) {
    m_slider->setTickInterval(interval);
}

void LabeledSlider::setPageStep(int step) {
    QAbstractSlider::setPageStep(step);
    m_slider->setPageStep(step);
}

void LabeledSlider::setValue(int value) {
    QAbstractSlider::setValue(value);
    if (value != m_slider->value()) {
        m_slider->setValue(value);
    }
    update_label();
}

void LabeledSlider::update_label() {
    QString text = m_label_format;
    text.replace("%v", locale().toString(value()));
    m_label->setText(text);
}

void LabeledSlider::update_label_size() {
    QString min_text = m_label_format;
    QString max_text = m_label_format;
    min_text.replace("%v", locale().toString(minimum()));
    max_text.replace("%v", locale().toString(maximum()));

    auto metrics = m_label->fontMetrics();
    int mintext_size = metrics.horizontalAdvance(min_text);
    int maxtext_size = metrics.horizontalAdvance(max_text);
    m_max_label_length = std::max(mintext_size, maxtext_size);

    m_label->setMinimumWidth(m_max_label_length);
}

} // namespace controls
