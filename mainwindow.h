#ifndef MAINWINDOW_H
#define MAINWINDOW_H


enum PRIORITY {
    IDLE = 0,
    LOWEST,
    BELOW_NORMAL,
    NORMAL,
    ABOVE_NORMAL,
    HIGHEST
};

enum STATUS {
    NOT_CREATED = 0,
    RUNNING,
    WAITING,
    DONE,
    SUSPENDED,
    KILLED,
    ENDED
};

#include <QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QList>
#include <QTimer>
#include <QRegularExpression>

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <mutex>
#include <thread>
#include <chrono>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void Suspend();         // Зупинити потік
    void Resume();          // Відновити потік
    void ChangePriority();  // Змінити пріоритет потоку
    void Kill();
        // Завершити потік
    void RefreshAction();   // Оновити стан потоку

    void StartWork();

    int threadsAmount;
    QList<HANDLE> hThreads;
    QList<DWORD> dwThreads;


private:
    Ui::MainWindow *ui;



    bool started = false;           // Чи запущена робота
    bool istext_printed = false;     // Чи був виведений текст

    std::chrono::system_clock::time_point start;  // Час початку роботи
};

#endif // MAINWINDOW_H
