#ifndef RESIZABLEQLABEL_H
#define RESIZABLEQLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

namespace controls {

class ResizableQLabel : public QLabel {
    Q_OBJECT

private:
    QPixmap m_pixmap;

public:
    explicit ResizableQLabel(QWidget* parent = nullptr);
    virtual int heightForWidth(int width) const override;
    virtual QSize sizeHint() const override;

    QPixmap scaledPixmap() const;

public slots:
    void setPixmap(const QPixmap&);
    void resizeEvent(QResizeEvent*) override;
};

} // namespace controls

#endif // RESIZABLEQLABEL_H
