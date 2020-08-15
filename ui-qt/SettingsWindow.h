#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QMainWindow>
#include <QSettings>

class TrayIcon;

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsWindow; }
QT_END_NAMESPACE

class SettingsWindow : public QMainWindow {
Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr, TrayIcon* trayIcon = nullptr);

    ~SettingsWindow() override;

private slots:
    void selectFont();

    void selectColor();

    void saveAndClose();

protected:
    void closeEvent(QCloseEvent* event) override;

    void saveSettings();

private:
    TrayIcon* trayIcon;
    Ui::SettingsWindow* ui;
    QSettings settings;
    QFont desktopFont, windowFont;

    bool desktopLyricsEnabled;

    void notifyChanges();
};

#endif // SETTINGSWINDOW_H
