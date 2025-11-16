#ifndef LABELEDSLIDER_H
#define LABELEDSLIDER_H

#include <QAbstractSlider>
#include <QLabel>
#include <QSlider>
#include <QWidget>

namespace controls {

class LabeledSlider : public QAbstractSlider {
    Q_OBJECT

    Q_PROPERTY(QString labelFormat READ labelFormat WRITE setLabelFormat NOTIFY
                   labelFormatChanged);
    Q_PROPERTY(Qt::Alignment labelAlignment READ labelAlignment WRITE setLabelAlignment
                   NOTIFY labelAlignmentChanged);
    Q_PROPERTY(QSlider::TickPosition tickPosition READ tickPosition WRITE setTickPosition)
    Q_PROPERTY(QSlider::TickPosition tickPosition READ tickPosition WRITE setTickPosition)
    Q_PROPERTY(int tickInterval READ tickInterval WRITE setTickInterval)

public:
    explicit LabeledSlider(QWidget* parent = nullptr);

    QString labelFormat() const;
    void setLabelFormat(const QString&);

    Qt::Alignment labelAlignment() const;
    void setLabelAlignment(Qt::Alignment);

    QSlider::TickPosition tickPosition() const;
    void setTickPosition(QSlider::TickPosition);

    int tickInterval() const;
    void setTickInterval(int);

    void setPageStep(int);

public slots:
    void setValue(int);

private:
    QSlider* m_slider;
    QLabel* m_label;

    QString m_label_format;
    int m_max_label_length;

private slots:
    void update_label();
    void update_label_size();

signals:
    void labelFormatChanged(const QString&);
    void labelAlignmentChanged(Qt::Alignment);
};

} // namespace controls

#endif // LABELEDSLIDER_H
