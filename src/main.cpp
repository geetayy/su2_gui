#include <QApplication>
#include <QSurfaceFormat>
#include <QStyleFactory>
#include <QPalette>

#include <QVTKOpenGLNativeWidget.h>

#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    // QApplication app(argc, argv);
    // app.setStyle(QStyleFactory::create("Fusion"));
    // QPalette palette = app.palette();
    // palette.setColor(QPalette::Window, QColor("#E7EBEF"));
    // palette.setColor(QPalette::WindowText, QColor("#26323D"));
    // palette.setColor(QPalette::Base, QColor("#F2F4F6"));
    // palette.setColor(QPalette::AlternateBase, QColor("#E7EBEF"));
    // palette.setColor(QPalette::Text, QColor("#000000"));
    // palette.setColor(QPalette::Button, QColor("#E1E6EB"));
    // palette.setColor(QPalette::ButtonText, QColor("#26323D"));
    // palette.setColor(QPalette::Highlight, QColor("#4A78A8"));
    // palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    // palette.setColor(QPalette::Mid, QColor("#B8C2CC"));
    // palette.setColor(QPalette::Dark, QColor("#9FAAB5"));
    // palette.setColor(QPalette::Light, QColor("#F8FAFB"));
    // app.setPalette(palette);


    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette palette = app.palette();
    palette.setColor(QPalette::Window, QColor("#EFEFEF"));
    palette.setColor(QPalette::WindowText, QColor("#202020"));
    palette.setColor(QPalette::Base, QColor("#FFFFFF"));
    palette.setColor(QPalette::AlternateBase, QColor("#F2F2F2"));
    palette.setColor(QPalette::Text, QColor("#202020"));
    palette.setColor(QPalette::Button, QColor("#E0E0E0"));
    palette.setColor(QPalette::ButtonText, QColor("#202020"));
    palette.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Highlight, QColor("#4A78A8"));
    palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Light, QColor("#FFFFFF"));
    palette.setColor(QPalette::Midlight, QColor("#F5F5F5"));
    palette.setColor(QPalette::Mid, QColor("#C8C8C8"));
    palette.setColor(QPalette::Dark, QColor("#A8A8A8"));
    palette.setColor(QPalette::Shadow, QColor("#808080"));
    app.setPalette(palette);


    MainWindow window;
    window.resize(1400, 900);
    window.show();

    return app.exec();
}