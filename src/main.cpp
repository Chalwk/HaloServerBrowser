// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}