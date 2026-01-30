#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QMouseEvent>

namespace controls {

class ClickableLabel : public QLabel {
    Q_OBJECT

public:
    ClickableLabel(QWidget* parent = nullptr) : QLabel(parent) {}

signals:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::MouseButton::LeftButton
            && rect().contains(event->pos())) {
            emit clicked();
        }
    }
};

} // namespace controls

#endif // CLICKABLELABLE_H
