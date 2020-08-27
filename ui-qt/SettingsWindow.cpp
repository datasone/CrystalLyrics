#include "SettingsWindow.h"
#include "ui_SettingsWindow.h"
#include "MainApplication.h"

#include <QFontDialog>
#include <QtWidgets>

inline QString fontString(const QFont& font) {
    return QString("%1 %2pt %3 %4").arg(font.family(), QString::number(font.pointSize()), font.bold() ? "Bold" : "",
                                        font.italic() ? "Italic" : "");
}

inline QString buttonColorStyleSheet(const QString& color) {
    return QString("QPushButton { background-color : %1; }").arg(color);
}

inline QString extractColorFromStyleSheet(const QString& styleSheet) {
    return styleSheet.split("background-color : ")[1].split(';')[0];
}

SettingsWindow::SettingsWindow(MainApplication *mainApp, QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SettingsWindow), mainApp(mainApp) {
    ui->setupUi(this);

    connect(ui->fontSelect, &QPushButton::clicked, this, &SettingsWindow::selectFont);

    connect(ui->backgroundColor, &QPushButton::clicked, this, &SettingsWindow::selectColor);
    connect(ui->lyricsTextColor, &QPushButton::clicked, this, &SettingsWindow::selectColor);
    connect(ui->lyricsTextPlayedColor, &QPushButton::clicked, this, &SettingsWindow::selectColor);

    connect(ui->windowFontSelect, &QPushButton::clicked, this, &SettingsWindow::selectFont);

    connect(ui->windowBackgroundColor, &QPushButton::clicked, this, &SettingsWindow::selectColor);
    connect(ui->windowLyricsTextColor, &QPushButton::clicked, this, &SettingsWindow::selectColor);
    connect(ui->windowLyricsTextPlayingColor, &QPushButton::clicked, this, &SettingsWindow::selectColor);

    connect(ui->saveButton, &QPushButton::clicked, this, &SettingsWindow::saveAndClose);

    // Some initialization
    ui->desktopLyrics->setChecked(true);
    ui->lyricsDoubleLine->setChecked(true);

    // Read settings
    auto desktopLyrics = settings.value("desktopLyrics", false);
    desktopLyricsEnabled = desktopLyrics.toBool();
    ui->desktopLyrics->setChecked(desktopLyricsEnabled);

    auto doubleLineDisplay = settings.value("doubleLineDisplay", false);
    (doubleLineDisplay.toBool() ? ui->lyricsDoubleLine : ui->lyricsSingleLine)->setChecked(true);

    auto desktopFontFamily = settings.value("desktopFont/family");
    if (desktopFontFamily.isNull()) {
        desktopFont = desktopFont.defaultFamily();
    } else {
        auto fontSize = settings.value("desktopFont/size", 16);
        auto fontWeight = settings.value("desktopFont/weight", QFont::Normal);
        auto fontItalic = settings.value("desktopFont/italic", false);
        desktopFont = QFont(desktopFontFamily.toString(), fontSize.toInt(), fontWeight.toInt(), fontItalic.toBool());
    }

    auto windowFontFamily = settings.value("windowFont/family");
    if (windowFontFamily.isNull()) {
        windowFont = windowFont.defaultFamily();
    } else {
        auto fontSize = settings.value("windowFont/size", 16);
        auto fontWeight = settings.value("windowFont/weight", QFont::Normal);
        auto fontItalic = settings.value("windowFont/italic", false);
        windowFont = QFont(windowFontFamily.toString(), fontSize.toInt(), fontWeight.toInt(), fontItalic.toBool());
    }

    ui->fontSelect->setText(fontString(desktopFont));
    ui->windowFontSelect->setText(fontString(windowFont));

    auto bgColor = settings.value("bgColor", "#99000000");
    ui->backgroundColor->setStyleSheet(buttonColorStyleSheet(bgColor.toString()));

    auto lyricsTextColor = settings.value("lyricsTextColor", "#FFFFFF");
    ui->lyricsTextColor->setStyleSheet(buttonColorStyleSheet(lyricsTextColor.toString()));

    auto lyricsTextPlayedColor = settings.value("lyricsTextPlayedColor", "#00FFDD");
    ui->lyricsTextPlayedColor->setStyleSheet(buttonColorStyleSheet(lyricsTextPlayedColor.toString()));

    auto windowBackgroundColor = settings.value("windowBackgroundColor", "#2B2B2B");
    ui->windowBackgroundColor->setStyleSheet(buttonColorStyleSheet(bgColor.toString()));

    auto windowLyricsTextColor = settings.value("windowLyricsTextColor", "#C0C0C0");
    ui->windowLyricsTextColor->setStyleSheet(buttonColorStyleSheet(windowLyricsTextColor.toString()));

    auto windowLyricsTextPlayingColor = settings.value("windowLyricsTextPlayingColor", "#E2FDCA");
    ui->windowLyricsTextPlayingColor->setStyleSheet(buttonColorStyleSheet(windowLyricsTextPlayingColor.toString()));

    auto ipcSocketName = settings.value("ipcSocketName", "clyricsocket");
    ui->ipcSocketName->setText(ipcSocketName.toString());

    auto autoConversionTCSC = settings.value("conversionTCSC", false);
    ui->conversionTCSC->setChecked(autoConversionTCSC.toBool());
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}

void SettingsWindow::closeEvent(QCloseEvent* event) {
    if (event->spontaneous()) {
        const QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Settings",
                                                                         "Do you want to save the settings?\n",
                                                                         QMessageBox::Cancel | QMessageBox::No |
                                                                         QMessageBox::Yes, QMessageBox::Yes);
        if (resBtn == QMessageBox::Cancel) {
            event->ignore();
        } else if (resBtn == QMessageBox::No) {
            event->accept();
        } else if (resBtn == QMessageBox::Yes) {
            saveSettings();
            event->accept();
        }
    } else {
        QWidget::closeEvent(event);
    }
}

void SettingsWindow::selectFont() {
    bool ok = false;
    auto *senderButton = qobject_cast<QPushButton*>(sender());
    const QFont& currentFont = (senderButton == ui->fontSelect) ? desktopFont : windowFont;
    const QFont selectedFont = QFontDialog::getFont(&ok, currentFont, this);
    if (ok) {
        if (senderButton == ui->fontSelect) {
            desktopFont = selectedFont;
        } else {
            windowFont = selectedFont;
        }
    }
    senderButton->setText(fontString(selectedFont));
}

void SettingsWindow::saveAndClose() {
    saveSettings();
    this->close();
}

void SettingsWindow::saveSettings() {
    settings.setValue("desktopLyrics", ui->desktopLyrics->isChecked());

    settings.setValue("doubleLineDisplay", ui->lyricsDoubleLine->isChecked());

    settings.setValue("desktopFont/family", desktopFont.family());
    settings.setValue("desktopFont/size", desktopFont.pointSize());
    settings.setValue("desktopFont/weight", desktopFont.weight());
    settings.setValue("desktopFont/italic", desktopFont.italic());

    settings.setValue("windowFont/family", windowFont.family());
    settings.setValue("windowFont/size", windowFont.pointSize());
    settings.setValue("windowFont/weight", windowFont.weight());
    settings.setValue("windowFont/italic", windowFont.italic());

    settings.setValue("bgColor", extractColorFromStyleSheet(ui->backgroundColor->styleSheet()));
    settings.setValue("lyricsTextColor", extractColorFromStyleSheet(ui->lyricsTextColor->styleSheet()));
    settings.setValue("lyricsTextPlayedColor", extractColorFromStyleSheet(ui->lyricsTextPlayedColor->styleSheet()));

    settings.setValue("windowBackgroundColor", extractColorFromStyleSheet(ui->windowBackgroundColor->styleSheet()));
    settings.setValue("windowLyricsTextColor", extractColorFromStyleSheet(ui->windowLyricsTextColor->styleSheet()));
    settings.setValue("windowLyricsTextPlayingColor",
                      extractColorFromStyleSheet(ui->windowLyricsTextPlayingColor->styleSheet()));

    settings.setValue("ipcSocketName", ui->ipcSocketName->text());

    settings.setValue("conversionTCSC", ui->conversionTCSC->isChecked());

    notifyChanges();
}

void SettingsWindow::selectColor() {
    auto *senderButton = qobject_cast<QPushButton*>(sender());
    const QColor currentColor = QColor(senderButton->styleSheet());
    const QColor color = QColorDialog::getColor(currentColor, nullptr, QString(), QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        senderButton->setStyleSheet(buttonColorStyleSheet(color.name(QColor::HexArgb)));
    }
}

void SettingsWindow::notifyChanges() {
    if (desktopLyricsEnabled != ui->desktopLyrics->isChecked())
        mainApp->reshowDesktopLyricsWindow(true, ui->desktopLyrics->isChecked());
    else mainApp->reshowDesktopLyricsWindow();
    mainApp->reshowLyricsWindow();
}
