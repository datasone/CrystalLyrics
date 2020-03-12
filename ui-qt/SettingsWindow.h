#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QMainWindow>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsWindow; }
QT_END_NAMESPACE

class SettingsWindow : public QMainWindow {
Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    ~SettingsWindow() override;

private slots:

    void desktopLyricDisplayModeToggled();

    void selectFont();

    void selectColor();

    void saveAndClose();

protected:
    void closeEvent(QCloseEvent* event) override;

    void saveSettings();

signals:

    void doubleLineAlignmentEnabled(bool);

private:
    Ui::SettingsWindow* ui;
    QSettings settings;
    QFont desktopFont, windowFont;
};

#endif // SETTINGSWINDOW_H
