#include "fonteditorwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ksFontGenerator"));
    QApplication::setOrganizationName(QStringLiteral("ksEditor"));

    FontEditorWindow window;
    window.show();

    return QApplication::exec();
}
