﻿#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <GL/glew.h>
#include "QtOpenGLWidgets/QOpenGLWidget"
#include <QTimer>
//#include <QOpenGLFunctions>
class GLWidget : public QOpenGLWidget
{
    Q_OBJECT
    QTimer* ter;
public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();
protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event) override;
    void closeEvent(QCloseEvent * e) override;
};

#endif
