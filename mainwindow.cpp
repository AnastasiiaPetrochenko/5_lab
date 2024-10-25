#include "mainwindow.h"
#include "ui_mainwindow.h"

struct Parameter {
    int thread_num;
    int start;
    int end;
};

CRITICAL_SECTION cs;
HANDLE hMutex;

QStringList text, outStrList;
QString pattern;

QList<int> thStatus;

QFile file("C:/Users/Admin/Desktop/politex/3_term/OperationSystems/lab5/Lab5_OS/lab5/Memoir.txt");


DWORD WINAPI WordSearcher(LPVOID lpParameter);
DWORD WINAPI WordSearcherM(LPVOID lpParameter);
DWORD WINAPI WordSearcherCS(LPVOID lpParameter);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::RefreshAction);
    timer->start(100);

    connect(ui->pushButton_start, &QPushButton::clicked, this, &MainWindow::StartWork);
    connect(ui->pushButton_suspend, &QPushButton::clicked, this, &MainWindow::Suspend);
    connect(ui->pushButton_resume, &QPushButton::clicked, this, &MainWindow::Resume);
    connect(ui->pushButton_kill, &QPushButton::clicked, this, &MainWindow::Kill);
    connect(ui->pushButton_change_priority, &QPushButton::clicked, this, &MainWindow::ChangePriority);

    if (!file.open(QIODevice::ReadOnly))
        throw 0;
}

MainWindow::~MainWindow() {
    if(ui->comboBox_type->currentIndex() == 1)
        CloseHandle(hMutex);
    else if(ui->comboBox_type->currentIndex() == 2)
        DeleteCriticalSection(&cs);
    for(int i = 0; i < threadsAmount; i++)
        CloseHandle(hThreads[i]);
    file.close();
    delete ui;
}

void MainWindow::RefreshAction() {
    if(!started) {
        return;
    }

    bool isAllDone = true;
    for(int i = 0; i < threadsAmount; i++) {
        if(thStatus[i] != DONE && thStatus[i] != KILLED) {
            isAllDone = false;
            break;
        }
    }

    if(!istext_printed && isAllDone) {
        ui->textEdit->setText(outStrList.join("\n"));
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        ui->lineEdit->setText("Time: " + QString::number(duration.count()) + " ms");
        istext_printed = true;
    }

    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setRowCount(0);

    for(int i = 0; i < threadsAmount; i++) {
        ui->tableWidget->insertRow(i);
        for(int j = 0; j < ui->tableWidget->columnCount(); j++) {
            QTableWidgetItem * item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            switch(j) {
            case 0:
                item->setText(QString::number(dwThreads[i])); // Thread ID
                break;

            case 1:
                item->setText(QString::number((quintptr)hThreads[i])); // Thread Handle (as integer)
                break;

            case 2:
                switch(thStatus[i]) {
                case NOT_CREATED:
                    item->setText(QString("Not Created"));
                    break;
                case RUNNING:
                    item->setText(QString("Running"));
                    break;
                case WAITING:
                    item->setText(QString("Waiting"));
                    break;
                case DONE:
                    item->setText(QString("Done"));
                    break;
                case SUSPENDED:
                    item->setText(QString("Suspended"));
                    break;
                case KILLED:
                    item->setText(QString("Killed"));
                    break;
                }
                break;

            case 3:
                switch(GetThreadPriority(hThreads[i])) {
                case THREAD_PRIORITY_IDLE:
                    item->setText(QString("IDLE"));
                    break;
                case THREAD_PRIORITY_LOWEST:
                    item->setText(QString("LOWEST"));
                    break;
                case THREAD_PRIORITY_BELOW_NORMAL:
                    item->setText(QString("BELOW_NORMAL"));
                    break;
                case THREAD_PRIORITY_NORMAL:
                    item->setText(QString("NORMAL"));
                    break;
                case THREAD_PRIORITY_ABOVE_NORMAL:
                    item->setText(QString("ABOVE_NORMAL"));
                    break;
                case THREAD_PRIORITY_HIGHEST:
                    item->setText(QString("HIGHEST"));
                    break;
                }
                break;
            }
            ui->tableWidget->setItem(i, j, item);
        }
    }
}


void MainWindow::StartWork(){
    ui->pushButton_start->setDisabled(true);

    start = std::chrono::high_resolution_clock::now();

    threadsAmount = ui->spinBox->value();

    hThreads.resize(threadsAmount);
    dwThreads.resize(threadsAmount);
    thStatus.resize(threadsAmount);

    pattern = ui->lineEdit_word->text();

    QStringList threadsList;

    QString str = file.readAll();
    text = str.split("\n"); // QRegularExpression("(\\r\\n)|(\\n\\r)|\\r|\\n"));
    switch(ui->comboBox_type->currentIndex()) {
    case 0 : {
        for(int i = 0; i < threadsAmount; i++ ) {
            threadsList << QString::number(i+1);
            Parameter * prm = new Parameter {i, (((int)text.size() / threadsAmount) * i), ((int)text.size() / threadsAmount) * (i+1)};
            hThreads[i] = CreateThread(
                NULL
                , 0
                , (LPTHREAD_START_ROUTINE) WordSearcher
                , prm
                , 0
                , &dwThreads[i]
                );
            if(hThreads[i] == NULL) {
                qDebug() << "CreateThread error: " << GetLastError();
            }
        }
        break;
    }
    case 1 : {
        hMutex = CreateMutex(NULL, FALSE, NULL);
        if(hMutex == NULL) {
            qDebug() << "CreateMutex error: " << GetLastError();
            return;
        }
        for(int i = 0; i < threadsAmount; i++ ) {
            threadsList << QString::number(i+1);
            Parameter * prm = new Parameter {i, (((int)text.size() / threadsAmount) * i), ((int)text.size() / threadsAmount) * (i+1)};
            hThreads[i] = CreateThread(
                NULL
                , 0
                , (LPTHREAD_START_ROUTINE) WordSearcherM
                , prm
                , 0
                , &dwThreads[i]	//зберігається ідентифікатор потоку в масиві
                );
            if(hThreads[i] == NULL) {
                qDebug() << "CreateThread error: " << GetLastError();
            }
        }
        break;
    }
    case 2 :{
        InitializeCriticalSection(&cs);
        for(int i = 0; i < threadsAmount; i++ ) {
            threadsList << QString::number(i+1);
            Parameter * prm = new Parameter {i, (((int)text.size() / threadsAmount) * i), ((int)text.size() / threadsAmount) * (i+1)};
            hThreads[i] = CreateThread(
                NULL
                , 0
                , (LPTHREAD_START_ROUTINE) WordSearcherCS
                , prm
                , 0
                , &dwThreads[i]
                );
            if(hThreads[i] == NULL) {
                qDebug() << "CreateThread error: " << GetLastError();
            }

        }
        break;
    }
    }
    ui->comboBox_kill_num->addItems(threadsList);
    ui->comboBox_priority_num->addItems(threadsList);
    ui->comboBox_suspend_num->addItems(threadsList);
    ui->comboBox_resume_num->addItems(threadsList);
    started = true;
}

DWORD WINAPI WordSearcher(LPVOID lpParameter) {
    Parameter * prm = (Parameter*)lpParameter;

    thStatus[prm->thread_num] = RUNNING;
    Sleep(10000);  // Затримка на 10 секунд
    for(int i = prm->start; i < prm->end; i++) {
        if(i < text.size() && text[i].contains(pattern)) {
            qDebug() << text[i];
        }
        //Sleep(500);  // Пауза на 500 мс
    }
    thStatus[prm->thread_num] = DONE;
    return TRUE;
}


DWORD WINAPI WordSearcherM(LPVOID lpParameter) {
    Parameter * prm = (Parameter*)lpParameter;
    DWORD dwWaitResult;
    BOOL IsLoop = TRUE;

    while(IsLoop) {
        dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
        switch(dwWaitResult) {
        case WAIT_OBJECT_0 : {
            IsLoop = FALSE;
            thStatus[prm->thread_num] = RUNNING;
            Sleep(10000);  // Затримка на 10 секунд
            for(int i = prm->start; i < prm->end; i++) {
                if(text[i].contains(pattern)) {
                    outStrList << text[i];
                }
                //Sleep(500);  // Пауза на 500 мс
            }
            thStatus[prm->thread_num] = DONE;
            if(!ReleaseMutex(hMutex)) {
                qDebug() << "Release mutex error: " << GetLastError();
            }
            break;
        }
        case WAIT_ABANDONED : {
            thStatus[prm->thread_num] = WAITING;
            return FALSE;
        }
        }
    }
    return TRUE;
}


DWORD WINAPI WordSearcherCS(LPVOID lpParameter) {
    Parameter * prm = (Parameter*)lpParameter;
    EnterCriticalSection(&cs);
    thStatus[prm->thread_num] = RUNNING;
    Sleep(10000);  // Затримка на 10 секунд

    for(int i = prm->start; i < prm->end; i++) {
        if(text[i].contains(pattern)) {
            outStrList << text[i];
        }
        //Sleep(500);  // Пауза на 500 мс
    }
    thStatus[prm->thread_num] = DONE;
    LeaveCriticalSection(&cs);
    return 1;
}




void MainWindow ::Suspend() {
    int index = ui->comboBox_suspend_num->currentIndex();
    SuspendThread(hThreads[index]);
    if (thStatus[index] != KILLED) {
        thStatus[index] = SUSPENDED;
        RefreshAction();  // Refresh UI to show updated state
        qDebug() << "Thread" << index << "suspended.";
    }
}

void MainWindow ::Resume() {
    int index = ui->comboBox_resume_num->currentIndex();
    ResumeThread(hThreads[index]);
    if (thStatus[index] != KILLED && thStatus[index] != DONE && thStatus[index] != WAITING) {
        thStatus[index] = RUNNING;
        RefreshAction();  // Update UI to show "Running" status
        qDebug() << "Thread" << index << "resumed.";
    }
}

void MainWindow::ChangePriority() {
    int index = ui->comboBox_priority_num->currentIndex();
    // Set priority based on selected value
    // After setting, update the UI to show priority status
    RefreshAction();
    qDebug() << "Thread" << index << "priority changed.";
}

void MainWindow::Kill() {
    int index = ui->comboBox_kill_num->currentIndex();
    TerminateThread(hThreads[index], 0);
    thStatus[index] = KILLED;
    RefreshAction();  // Refresh to show "Killed" status
    qDebug() << "Thread" << index << "terminated.";
}


