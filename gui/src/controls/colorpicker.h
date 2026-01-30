#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <optional>

#include <QColor>
#include <QLabel>
#include <QPixmap>
#include <QWidget>

#include "clickablelabel.h"

namespace controls {

class ColorPicker : public QWidget {
    Q_OBJECT

    Q_PROPERTY(QColor color MEMBER m_color READ color WRITE setColor NOTIFY colorChanged);
    Q_PROPERTY(bool allowAlpha MEMBER m_allow_alpha READ allowAlpha WRITE setAllowAlpha
                   NOTIFY allowAlphaChanged);

public:
    explicit ColorPicker(QWidget* parent = nullptr);

    QColor color() const { return m_color; }
    bool allowAlpha() const { return m_allow_alpha; }

public slots:
    void setValue(const QColor& color) { setColor(color); }
    void setColor(const QColor& color);
    void setAllowAlpha(bool allow);

private slots:
    void openPicker();

signals:
    void colorChanged(const QColor&);
    void allowAlphaChanged(bool allow);

private:
    ClickableLabel* m_lbl_color;

    QColor m_color = QColor(0, 0, 0);
    bool m_allow_alpha = true;

    static std::optional<QPixmap> s_checkerboard_pixmap;
    static QPixmap& get_checkerboard_pixmap();
};

} // namespace controls

#endif // COLORPICKER_H
