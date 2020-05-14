#include "TrayIcon.h"
#include "utils.h"
#include "RunGuard.h"

#include <QApplication>

// TODO: Lyric offset
int main(int argc, char* argv[]) {
    RunGuard guard("DzHEvHYGGC");
    if (!guard.tryToRun())
        return 0;

    QCoreApplication::setOrganizationName("datasone");
    QCoreApplication::setOrganizationDomain("datasone.moe");
    QCoreApplication::setApplicationName("CrystalLyrics");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(":/image/icon.png"));
    QApplication::setQuitOnLastWindowClosed(false);

#ifdef Q_OS_WIN
    QFont font("Microsoft YaHei UI", 9); // Yes the MS Shell Dlg 2 will fallback to SimSun regardless of the SystemLink
    font.setStyleHint(QFont::SansSerif);
    QApplication::setFont(font);
#endif

    TrayIcon trayIcon;

    return QApplication::exec();
}
