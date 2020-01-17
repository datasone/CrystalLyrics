//
// Created by datasone on 3/1/2020.
//

#ifndef CRYSTALLYRICS_WINDOW_H
#define CRYSTALLYRICS_WINDOW_H


#include <QWidget>
#include <QPushButton>

class Window : public QWidget {

Q_OBJECT
public:
    explicit Window(QWidget *parent = nullptr);

signals:

    void counterReached();

private:
    QPushButton *m_button;
    int m_counter;
private slots:

    void specialButtonClicked(bool checked);
};


#endif //CRYSTALLYRICS_WINDOW_H
