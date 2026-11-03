#include "convergence_widget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPolygonF>

#include <algorithm>
#include <cmath>

ConvergenceWidget::ConvergenceWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(170);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ConvergenceWidget::addValue(double value)
{
    if (value <= 0.0)
        return;

    values.push_back(value);

    if (values.size() > 2000)
        values.remove(0, values.size() - 2000);

    update();
}

void ConvergenceWidget::clearValues()
{
    values.clear();
    update();
}

void ConvergenceWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#11151b"));

    const int left = 48;
    const int right = 14;
    const int top = 26;
    const int bottom = 28;

    const QRect plot(left, top,
                     width() - left - right,
                     height() - top - bottom);

    p.setPen(QPen(QColor("#303844"), 1));
    p.drawRect(plot);

    p.setPen(QPen(QColor("#242b34"), 1));
    for (int i = 1; i < 5; ++i)
    {
        const int y = plot.top() + i * plot.height() / 5;
        p.drawLine(plot.left(), y, plot.right(), y);
    }

    p.setPen(QPen(QColor("#b8c2cf"), 1));
    p.drawText(10, 17, "CONVERGENCE / RESIDUAL");
    p.drawText(5, plot.top() + 4, "10⁰");
    p.drawText(2, plot.bottom(), "10⁻¹⁶");
    p.drawText(plot.left() + plot.width() / 2 - 25,
               height() - 7,
               "Iteration");

    if (values.size() < 2)
    {
        p.setPen(QColor("#6f7a88"));
        p.drawText(plot.center() - QPoint(45, -4),
                   "Waiting for residuals...");
        return;
    }

    double maxLog = 0.0;
    const double minLog = -16.0;

    for (double value : values)
        maxLog = std::max(maxLog, std::log10(value));

    maxLog = std::ceil(maxLog);
    if (maxLog < 0.0)
        maxLog = 0.0;

    p.setPen(QPen(QColor("#55c7ff"), 2));

    QPolygonF poly;
    poly.reserve(values.size());

    for (int i = 0; i < values.size(); ++i)
    {
        const double logValue = std::log10(values[i]);
        const double t = values.size() == 1
                             ? 0.0
                             : static_cast<double>(i) /
                                   static_cast<double>(values.size() - 1);

        const double u = (logValue - minLog) / (maxLog - minLog);
        const double clamped = std::clamp(u, 0.0, 1.0);

        const double x = plot.left() + t * plot.width();
        const double y = plot.bottom() - clamped * plot.height();

        poly << QPointF(x, y);
    }

    p.drawPolyline(poly);
}
