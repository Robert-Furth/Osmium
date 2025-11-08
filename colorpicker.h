#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QColor>
#include <QWidget>

namespace Ui {
class ColorPicker;
}

namespace controls {

class ColorPicker : public QWidget {
    Q_OBJECT

    Q_PROPERTY(QColor color MEMBER m_color READ color WRITE setColor NOTIFY valueChanged);
    Q_PROPERTY(bool allowAlpha MEMBER m_allow_alpha READ allowAlpha WRITE setAllowAlpha
                   NOTIFY allowAlphaChanged);

public:
    explicit ColorPicker(QWidget* parent = nullptr);
    ~ColorPicker();

    QColor color() const { return m_color; }
    bool allowAlpha() const { return m_allow_alpha; }

public slots:
    void setValue(const QColor& color) { setColor(color); }
    void setColor(const QColor& color);
    void setAllowAlpha(bool allow);

private slots:
    void openPicker();

signals:
    void valueChanged(const QColor&);
    void allowAlphaChanged(bool allow);

private:
    Ui::ColorPicker* ui;

    QColor m_color = QColor(0, 0, 0);
    bool m_allow_alpha = true;
};

} // namespace controls

#endif // COLORPICKER_H
