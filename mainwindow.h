#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QEvent>
#include "qcustomplot.h"
#include <QElapsedTimer>
#include <chrono>
#include <thread>
#include <QString>
#include "Processitem.h"
#include <cstddef>
#define _WIN32_DCOM
#include<windows.h>
#include<memory>
#define gb (1024.0*1024.0*1024.0)
#define mb (1024.0*1024.0)
#define kb (1024.0)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

 enum Computer_device{
    Memory,Cpu
 };

private:

void draw(long double , Computer_device );
void processDraw(long double , Computer_device );

void add_sample(QVector<double>&,double item);

void get_mem(std::byte* = nullptr);
void getProcess_mem(std::byte* = nullptr);

void get_cpuClock(std::byte* = nullptr);
void getProcess_cpuClock(std::byte* = nullptr);




double get_mem_val();



void reset_sampleRate(Computer_device type);

void do_test();

void processResource();

private:


QVector<Process> device_processes;
void getProcessInfo(DWORD processID,Process& old);
auto updateProcessInfo();
QTreeWidget* processInfoTree;
void updateProcessInfoTree();
QList<QTreeWidgetItem*> selectedItems;
bool selectedItemsInUse = false;

private:
QComboBox* process_ComboBox;
Process currentProcess = Process("",0,0,0,QIcon(QPixmap(5,5)));
std::map<QString,double> processMax_MemoryUsage;


void updateProcessComboBox();





private:
    enum  SortMode{
        NameSortAZ, MemorySortAZ, CpuSortAZ, ProcessIDSortAZ, NameSortZA,MemorySortZA, CpuSortZA, ProcessIDSortZA,NoSort
    };
    SortMode currentSortMode= SortMode::NoSort;

private:

QTimer* m_timer;

double time_value = 0.0;
double compare_sample = 0.0;


unsigned char stayOnTop = 0;



private:

    QVector<QVector<double>> device_samples;
    QVector<QVector<double>> process_samples;

    std::thread backround;
    int clocks = 0;

private slots:

void Update();






void on_comboBox_currentIndexChanged(const QString &arg1);

void on_actionStay_on_top_triggered();

void on_actionStay_on_top_toggled(bool arg1);

void on_pushButton_clicked();

void on_actionSave_as_triggered();

void on_pushButton_2_clicked();

void tableHeader_clicked(int column);

private:

QLabel* resourceUsageLabel;

std::byte currentResourceUsage;

private:


    Ui::MainWindow *ui;
    QCustomPlot* customPlot;
    QCustomPlot* processCustomPlot;

    int sample_time = 30000;

    int mem_sampleRate = 10000;
   
    int cpu_sampleRate =  100;

    int cpu_SamplingRate = 100;

  


   //QVector<double> samples = QVector<double>(sampleRate); // initialize with entries 0..100
   QVector<double>  x = QVector<double>(mem_sampleRate);
    QVector<double> multi_sample= QVector<double>();
   MEMORYSTATUSEX statex{};

Computer_device current_device;

unsigned long long total_ram = 0;
long long total_cpuClock = 4*gb;


//image stuff
private:

QImage m_plot;

bool imageIsSafe =false;

bool capture_plot = 0;

};
#endif // MAINWINDOW_H
