#ifndef PREVIEWER_H
#define PREVIEWER_H

#include <optional>

#include <QObject>
#include <QPixmap>
#include <QWidget>

#include "../renderargs.h"
#include "../scoperenderer.h"

namespace controls {

class Previewer : public QWidget {
    Q_OBJECT
public:
    explicit Previewer(QWidget* parent = nullptr);

    void paintEvent(QPaintEvent* event) override;

public slots:
    void update_args(const GlobalArgs& global_args, const QList<ChannelArgs>& chan_args);
    void set_pixmap(const QPixmap& pixmap);
    void clear_pixmap();

signals:

private:
    std::optional<PreviewRenderer> m_renderer;
    std::optional<QPixmap> m_pixmap;
};

} // namespace controls

#endif // PREVIEWER_H
