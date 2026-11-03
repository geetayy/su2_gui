#pragma once

#include <QWidget>
#include <QVector>

class ConvergenceWidget : public QWidget
{
public:
    explicit ConvergenceWidget(QWidget *parent = nullptr);

    void addValue(double value);
    void clearValues();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> values;
};
