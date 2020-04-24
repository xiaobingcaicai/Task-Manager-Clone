#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cstdlib>

#include <iostream>
#include <thread>

#include "TCHAR.h"
#include "pdh.h"
#include <psapi.h>

#include <QFileDialog>
#define _WIN32_DCOM
#include <QtAlgorithms>
#include <QtMath>
#include <QtWin>
#include <Wbemidl.h>
#include <comdef.h>
#include <tlhelp32.h>


#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma comment(lib, "wbemuuid.lib")
#endif

static float CalculateCPULoad(unsigned long long idleTicks,
                              unsigned long long totalTicks) {
  static unsigned long long _previousTotalTicks = 0;
  static unsigned long long _previousIdleTicks = 0;

  unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
  unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

  float ret =
      1.0F - ((totalTicksSinceLastTime > 0)
                  ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime
                  : 0);

  _previousTotalTicks = totalTicks;
  _previousIdleTicks = idleTicks;
  return ret;
}

static unsigned long long FileTimeToInt64(const FILETIME &ft) {
  return (((unsigned long long)(ft.dwHighDateTime)) << 32) |
         ((unsigned long long)ft.dwLowDateTime);
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in
// between You'll need to call this at regular intervals, since it measures the
// load between the previous call and the current one.  Returns -1.0 on error.
float GetCPULoad() {
  FILETIME idleTime;
  FILETIME kernelTime;
  FILETIME userTime;
  return GetSystemTimes(&idleTime, &kernelTime, &userTime)
             ? CalculateCPULoad(FileTimeToInt64(idleTime),
                                FileTimeToInt64(kernelTime) +
                                    FileTimeToInt64(userTime))
             : -1.0F;
}

DWORD getParentPID(DWORD pid) {
  HANDLE h = nullptr;
  PROCESSENTRY32 pe{0,0,0,0,0,0,0,0,0,{0,0}};
  DWORD ppid = 0;
  pe.dwSize = sizeof(PROCESSENTRY32);
  h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (Process32First(h, &pe)) {
    do {
      if (pe.th32ProcessID == pid) {
        ppid = pe.th32ParentProcessID;
        break;
      }
    } while (Process32Next(h, &pe));
  }
  CloseHandle(h);
  return (ppid);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);


  customPlot = ui->tab;
 processCustomPlot = ui->tab_3;
  ui->tabWidget->setTabText(0, "Performance");
  ui->tabWidget->setTabText(1, "Processes");
  ui->tabWidget->setTabText(2, "Process performance");

  resourceUsageLabel = ui->label_2;
  customPlot->addGraph();
  processCustomPlot->addGraph();
  processInfoTree = ui->treeWidget;
  process_ComboBox = ui->comboBox_2;
  process_ComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

  // set selection mode
  processInfoTree->setSelectionMode(
      QAbstractItemView::SelectionMode::ExtendedSelection);
  // setup headers
  processInfoTree->setColumnCount(4);

  QList<QString> headers{QString("Name"), QString("Memory"), QString("Cpu"),
                         QString("ProcessId")};
  QTreeWidgetItem *headerItem =
      new QTreeWidgetItem((QTreeWidgetItem *)nullptr, headers);
  headerItem->setSelected(true);
  processInfoTree->setHeaderItem(headerItem);
  // keep window on top
  setWindowFlags(this->windowFlags() | Qt::X11BypassWindowManagerHint |
                 Qt::WindowStaysOnTopHint);

  /*connect slot functions*/
  connect(ui->treeWidget->header(), SIGNAL(sectionDoubleClicked(int)), this,
          SLOT(tableHeader_clicked(int)));

  // update it the first frame so there is not a small time where it is blank
  updateProcessInfoTree();

  // get total memory

  statex.dwLength = sizeof(statex);

  GlobalMemoryStatusEx(&statex);

  total_ram = statex.ullTotalPhys;

  // fill samples

  double current_memValue =
      (((double)total_ram - (double)statex.ullAvailPhys) / gb);

  // set enum
  current_device = Memory;
  backround = std::thread();

  // set up device sample vectors
  device_samples.push_back(QVector<double>(mem_sampleRate, current_memValue));
  device_samples.push_back(
      QVector<double>(cpu_sampleRate, GetCPULoad() * 100.0));


  process_samples.push_back(QVector<double>(mem_sampleRate, 0));
  process_samples.push_back(
      QVector<double>(cpu_sampleRate, GetCPULoad() * 100.0));

  // set up vars

  // set up timer

  m_timer = new QTimer(this);

  QTimer::singleShot(1, this, SLOT(Update()));

  m_timer->start(1);

  for (int i = 0; i < mem_sampleRate; ++i) {
    x[i] = ((double)i / (mem_sampleRate));
  }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::draw(long double top, Computer_device type) {

  QVector<double> samples = device_samples[type];

  // create graph and assign data to it:

  QColor a;
  a.setRgb(160, 3, 170, 70);

  customPlot->graph()->setBrush(a);
  customPlot->graph(0)->setData(x, samples);

  // give the axes some labels:
  customPlot->yAxis->setLabel("Total Use");
  customPlot->xAxis->setTicks(false);

  // set axes ranges, so we see all data:
  customPlot->xAxis->setRange(0.0, 1.0);
  customPlot->yAxis->setRange(0.0, top);
  customPlot->replot();

  if (capture_plot) {
    m_plot = customPlot->toPixmap(this->width(), this->height()).toImage();

    capture_plot = false;
  }

  multi_sample.clear();
}

void MainWindow::processDraw(long double top, Computer_device type)
{

    QVector<double> samples = process_samples[type];

    // create graph and assign data to it:

    QColor a;
    a.setRgb(160, 3, 170, 70);

    processCustomPlot->graph()->setBrush(a);
    processCustomPlot->graph(0)->setData(x, samples);

    // give the axes some labels:
    processCustomPlot->yAxis->setLabel("Total Use");
    processCustomPlot->xAxis->setTicks(false);

    // set axes ranges, so we see all data:
    processCustomPlot->xAxis->setRange(0.0, 1.0);
    processCustomPlot->yAxis->setRange(0.0, top);
    processCustomPlot->replot();
    if (capture_plot) {
      m_plot = processCustomPlot->toPixmap(this->width(), this->height()).toImage();

      capture_plot = false;
    }

}

void MainWindow::add_sample(QVector<double> &vec, double item) {
  item = abs(item);

  vec.pop_back();

  vec.insert(0, item);
}

/*
 *   sample = sin(time/2.0)*0.5+0.5;
 *  sample *= total_ram/gb;
 * double time = (double)QTime::currentTime().msecsSinceStartOfDay()/1000.0;
 */

// get memory
void MainWindow::get_mem(std::byte *currentValue) {

  double sample = 0;

  GlobalMemoryStatusEx(&statex);

  sample = double(total_ram - statex.ullAvailPhys) / gb;
  if (currentValue != nullptr) {
    *currentValue = static_cast<std::byte>(
        round((total_ram - statex.ullAvailPhys) / double(total_ram) * 100.0));
  }

  add_sample(device_samples[0], sample);
}

void MainWindow::getProcess_mem(std::byte *currentValue)
{
    double sample = 0;
    if(device_processes.empty()){return;}


    sample = double(currentProcess.memoryUsage());
    if (currentValue != nullptr) {
      *currentValue = static_cast<std::byte>(
          round((total_ram - statex.ullAvailPhys) / double(processMax_MemoryUsage[currentProcess.name()]) * 100.0));
    }

    add_sample(process_samples[0], sample);
}

void MainWindow::getProcessInfo(DWORD processID, Process &old) {

  TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

  // Get a handle to the process.

  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
  _PROCESS_MEMORY_COUNTERS_EX pmc{};

  old.SetId(processID);
  // Print the process identifier.

  // Get the process name.

  if (nullptr != hProcess) {

    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {

      GetModuleBaseName(hProcess, hMod, szProcessName,
                        sizeof(szProcessName) / sizeof(TCHAR));
    }
    
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc,
                             sizeof(pmc))) {

      old.SetmemoryUsage(pmc.PrivateUsage / (mb));
    };

    auto temp = QString::fromUtf16((const ushort *)szProcessName);
    old.Setname(temp);

   if(processMax_MemoryUsage.find(temp)!=processMax_MemoryUsage.end())
   {
        if(old.memoryUsage() > processMax_MemoryUsage[temp])
        processMax_MemoryUsage[temp]=old.memoryUsage()*1.25;
   }
   else
   {
       processMax_MemoryUsage[temp] = old.memoryUsage();
   }
   /// old.UpdatecpuUsage();
    if (GetModuleFileNameEx(hProcess, hMod, szProcessName, MAX_PATH)) {
      HICON icon = ExtractIcon((HINSTANCE)hProcess, szProcessName, 0);
      if (icon) {
        old.SetIcon(QIcon(QtWin::fromHICON(icon)));
      } else {
        icon = LoadIcon(nullptr, IDI_APPLICATION);
        old.SetIcon(QIcon(QtWin::fromHICON(icon)));
      }
      DestroyIcon(icon);
    }

  } else {
    old.Setname("<NULL>");
  }


  // Release the handle to the process.

  CloseHandle(hProcess);
}


auto MainWindow::updateProcessInfo() {

  DWORD aProcesses[1024]; DWORD cbNeeded; DWORD cProcesses;
  unsigned int i;

  if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
    qWarning() << "Error could not enumerate processes\n";
  }

  // Calculate how many process identifiers were returned.
  cProcesses = cbNeeded / sizeof(DWORD);
  auto &processes = device_processes;
  std::map<int, Process> pmap;

  if (processes.empty()) {
    processes.resize(cProcesses);
    processes.fill(Process(), processes.size());
  } else {
    processes.resize(cProcesses);
  }

  int k = 0;

  for (i = 0; i < cProcesses; i++) {
    getProcessInfo(aProcesses[i], processes[k]);
    if (processes[k].memoryUsage() != 0 && processes[k].name() != "<unknown>" &&
        processes[k].name() != "<NULL>") {
      k++;
    }
  }

  processes.resize(k);
  for (const auto& process : processes) {
    pmap[process.Id()] = process;
  }

	
  QVector<Process> newProcesses;
  // a way to avoid using push_back()
  i = 0;
  for (const auto& j : pmap) {

    if (j.second.child()!=1) {
      newProcesses.append(j.second);
    }
    i++;
  }

  device_processes = newProcesses;
}

void MainWindow::updateProcessInfoTree() {

  // only update every 500 milliseconds and if the current tab is the tables



  if (clocks % 500 == 0 && ui->tabWidget->currentIndex() != 0) {
    updateProcessInfo();
    int len = device_processes.size();
    int len2 = processInfoTree->topLevelItemCount();
    int len3 = len - len2;
    /*add or remove items based off new length*/
    if (len3 > 0) {
      for (int i = 0; i < len3; i++) {
        new QTreeWidgetItem(
            processInfoTree, QStringList{" ", " ", " ", " "});
      }
    } else if (len3 < 0) {
      for (int i = len2 - 1; i > (len2 - 1) - abs(len3); --i) {
        delete processInfoTree->takeTopLevelItem(i);
      }
    }
    auto sorted_device_processes = device_processes;
    /*Sort table based on current sort mode*/
    switch (currentSortMode) {

    default:
      break;
    case SortMode::NameSortAZ:
      std::sort(
          sorted_device_processes.begin(), sorted_device_processes.end(),
          [](Process &a, Process &b) -> bool { return a.name() < b.name(); });
      break;
    case SortMode::NameSortZA:
      std::sort(
          sorted_device_processes.begin(), sorted_device_processes.end(),
          [](Process &a, Process &b) -> bool { return a.name() > b.name(); });
      break;
    case CpuSortAZ:
      std::sort(sorted_device_processes.begin(), sorted_device_processes.end(),
                [](Process &a, Process &b) -> bool {
                  return a.cpuUsage() < b.cpuUsage();
                });
      break;
    case CpuSortZA:
      std::sort(sorted_device_processes.begin(), sorted_device_processes.end(),
                [](Process &a, Process &b) -> bool {
                  return a.cpuUsage() > b.cpuUsage();
                });
      break;
    case MemorySortAZ:
      std::sort(sorted_device_processes.begin(), sorted_device_processes.end(),
                [](Process &a, Process &b) -> bool {
                  return a.memoryUsage() < b.memoryUsage();
                });
      break;
    case MemorySortZA:
      std::sort(sorted_device_processes.begin(), sorted_device_processes.end(),
                [](Process &a, Process &b) -> bool {
                  return a.memoryUsage() > b.memoryUsage();
                });
      break;
    case ProcessIDSortAZ:
      std::sort(sorted_device_processes.begin(), sorted_device_processes.end(),
                [](Process &a, Process &b) -> bool { return a.Id() < b.Id(); });
      break;
    case ProcessIDSortZA:
      std::sort(sorted_device_processes.begin(), sorted_device_processes.end(),
                [](Process &a, Process &b) -> bool { return a.Id() > b.Id(); });
      break;
    case NoSort:
      break;
    }

    for (int i = 0; i < len; ++i) {

      auto process = sorted_device_processes[i];

      QTreeWidgetItem item5;

      QStringList currentRow(QString(process.name()));
      currentRow.append(QString::number(process.memoryUsage()) + "Mb");
      currentRow.append(QString::number(process.cpuUsage()) + "%");
      currentRow.append(QString::number(process.Id()));
      item5 = QTreeWidgetItem((QTreeWidget *)nullptr, currentRow);

      item5.setIcon(0, process.icon());
      // update item at index i in the tree
      *processInfoTree->topLevelItem(i) = item5;
      processInfoTree->topLevelItem(i)->takeChildren();
      for (const auto& child : process.children()) {

        QStringList currentChildRow(QString(child.name()));
        currentChildRow.append(QString::number(child.memoryUsage()) + "Mb");
        currentChildRow.append(QString::number(child.cpuUsage()) + "%");
        currentChildRow.append(QString::number(child.Id()));
        auto *item6 =
            new QTreeWidgetItem((QTreeWidget *)nullptr, currentChildRow);
        item6->setIcon(0, child.icon());
        processInfoTree->topLevelItem(i)->addChild(item6);
      }
    }
    //update process combo box
    updateProcessComboBox();
  }
}

void MainWindow::updateProcessComboBox()
{
    int numberOfProcesses = device_processes.size();
    int len = numberOfProcesses;
    int len2 = process_ComboBox->count();
    int len3 = len - len2;
    /*add or remove items based off new length*/
    if (len3 > 0) {
      for (int i = 0; i < len3; i++) {
        process_ComboBox->addItem("");
      }
    } else if (len3 < 0) {
      for (int i = len2 - 1; i > (len2 - 1) - abs(len3); --i) {
          process_ComboBox->removeItem(i);
      }
    }

    for(auto i = 0; i < numberOfProcesses; ++i)
    {
        process_ComboBox->setItemText(i,device_processes[i].name());
    }
  int currentProcessIndex = process_ComboBox->findText(currentProcess.name());
  if(currentProcessIndex^-1)//if it finds the currentProcess
  {
    process_ComboBox->setCurrentIndex(currentProcessIndex);
  }



}

void MainWindow::get_cpuClock(std::byte *currentValue) {

  double sample = 0;

  if (clocks % cpu_SamplingRate == 0) {

    sample = GetCPULoad() * 100.0;
    if (clocks % 2 == 0) {
      if (currentValue != nullptr) {
        *currentValue = static_cast<std::byte>(sample);
      }
      add_sample(device_samples[1], sample);
    }

  }
}
void MainWindow::getProcess_cpuClock(std::byte *currentValue)
{
    double sample = 0;
    if(device_processes.empty()){return;}

    if (clocks % cpu_SamplingRate == 0) {
      sample = currentProcess.cpuUsage() * 100.0;
      if (clocks % 2 == 0) {
        if (currentValue != nullptr) {
          *currentValue = static_cast<std::byte>(sample);
        }
        add_sample(process_samples[1], sample);
      }
    }
}

void MainWindow::Update() {
    //increment clock
  clocks++;
  get_cpuClock((current_device == Cpu) ? &currentResourceUsage : nullptr);

  get_mem((current_device == Memory) ? &currentResourceUsage : nullptr);

  // if stayOnTop is enabled make the window Stay on Top
  if (stayOnTop == 0) {
    stayOnTop = 2;
    setWindowFlags(this->windowFlags() | Qt::X11BypassWindowManagerHint |
                   Qt::WindowStaysOnTopHint);

  } else if (stayOnTop == 1) {
    stayOnTop = 2;
    this->setWindowFlags((this->windowFlags() & ~Qt::WindowStaysOnTopHint) &
                         ~Qt::X11BypassWindowManagerHint);
  }



  // update tree and graph
  updateProcessInfoTree();
  processResource();




getProcess_cpuClock(nullptr);

getProcess_mem(nullptr);


//update the current process from process_ComboBox
currentProcess = device_processes[std::max(0,process_ComboBox->currentIndex())];
getProcessInfo(currentProcess.Id(),currentProcess);




 
  if (!selectedItemsInUse) {
    selectedItems = processInfoTree->selectedItems();
  }





  // draw/hide button based off conditions
  switch(ui->tabWidget->currentIndex()){
  case 0:{
    ui->pushButton_2->hide();
    ui->comboBox_2->hide();
    break;
  } case 1:{
    ui->pushButton_2->show();
    ui->comboBox_2->hide();
    break;
  }
  case 2:{
      ui->comboBox_2->show();
      ui->pushButton_2->hide();
      break;
  }
}




  // update every millisecond
  QTimer::singleShot(1, this, SLOT(Update()));
}
void MainWindow::do_test() {
  static std::vector<char> alloc;
  static QElapsedTimer __timer;
 long double time = __timer.nsecsElapsed()/(long double)(1e9);
long double mema = sinl(time)*0.5l+0.5l;
  mema *= mb*512.;
  //qWarning() << mema << '\n';
 alloc.resize(mema);
}

void MainWindow::processResource() {

  GlobalMemoryStatusEx(&statex);

  // redraw everytime update is called;
  resourceUsageLabel->setText(
      QString::number((unsigned char)currentResourceUsage) + "%");

  // process resource use
  switch (current_device) {
  case Memory: {

    draw(static_cast<double>(total_ram) / gb, Memory);

    if(device_processes.empty() ||  ui->tabWidget->currentIndex()^2) {return;}


    processDraw(processMax_MemoryUsage[currentProcess.name()]
            , Memory);

    break;
  }

  case Cpu: {

    draw(100.0, Cpu);
    if(device_processes.empty() ||  ui->tabWidget->currentIndex()^2) {return;}
    processDraw(100.0, Cpu);
    break;
  }
  }
}

double MainWindow::get_mem_val() {
  double sample = 0;

  GlobalMemoryStatusEx(&statex);

  sample = double(total_ram - (long long)statex.ullAvailPhys) / gb;
  return sample;
}

void MainWindow::reset_sampleRate(Computer_device type) {

  int temp_rate = (type == Memory) ? mem_sampleRate : cpu_sampleRate;

  x = QVector<double>(temp_rate, 0);

  for (int i = 0; i < temp_rate; ++i) {
    x[i] = ((double)i / (temp_rate));
  }
}

void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1) {

  // Memory

  if (arg1 == "Memory") {

    reset_sampleRate(Memory);
    ui->label->setText("Memory Usage");

    current_device = Memory;
  }

  if (arg1 == "Cpu") {

    reset_sampleRate(Cpu);
    device_samples[1].fill(GetCPULoad() * 100.0, cpu_sampleRate);

    ui->label->setText("Cpu Usage");

    current_device = Cpu;
  }
}

void MainWindow::on_actionStay_on_top_triggered() {}

void MainWindow::on_actionStay_on_top_toggled(bool arg1) {
  stayOnTop = (arg1) ? 0 : 1;
}

void MainWindow::on_pushButton_clicked() {
  imageIsSafe = true;

  switch (ui->tabWidget->currentIndex()) {
  case 0: {
    capture_plot = true;
    break;
  }
  case 1: {

    break;
  }

  default:
    break;
  }
}

void MainWindow::on_actionSave_as_triggered() {
  if (imageIsSafe) {
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Image File"), QString(), tr("Images (*.png)"));
    if (!fileName.isEmpty()) {
      m_plot.save(fileName);
    }
  }
}

void MainWindow::on_pushButton_2_clicked() {
  selectedItemsInUse = true;
  if (selectedItems.empty()) {
    return;
  }
  std::vector<size_t> processIds(selectedItems.size());
  int i = 0;
  for (const QTreeWidgetItem *treeItem : selectedItems) {
    // if item selected is not a process id column continue
    processIds[i] = treeItem->text(3).toULongLong();
    i++;
  }
  // delete all the selected process
  i = 0;
  for (const auto &processId : processIds) {
    /*magic WIN32 API stuff*/
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (nullptr != hProcess) {

      if (processId == GetCurrentProcessId()) {
#if 0
          int  k;
          int start = QTime::currentTime().msecsSinceStartOfDay();
                float A =
                    0, B = 0, i, j, z[1760]; char b[
                        1760]; printf("\x1b[2J"); for (;;
                            ) {
                            this->setWindowOpacity(1.0f/((QTime::currentTime().msecsSinceStartOfDay()-start) / 1000.0f));
                            memset(b, 32, 1760); memset(z, 0, 7040)
                                ; for (j = 0; 6.28 > j; j += 0.07)for (i = 0; 6.28
 > i; i += 0.02) {
                                float c = sin(i), d = cos(j), e =
                                    sin(A), f = sin(j), g = cos(A), h = d + 2, D = 1 / (c *
                                        h * e + f * g + 5), l = cos(i), m = cos(B), n = sin(B), t = c * h * g - f * e; int x = 40 + 30 * D *
                                    (l * h * m - t * n), y = 12 + 15 * D * (l * h * n
                                        + t * m), o = x + 80 * y, N = 8 * ((f * e - c * d * g
                                            ) * m - c * d * e - f * g - l * d * n); if (22 > y &&
                                                y > 0 && x > 0 && 80 > x && D > z[o]) {
                                    z[o] = D;;; b[o] =
                                        ".,-~:;=!*#$@"[N > 0 ? N : 0];
                                }
                            }/*#****!!-*/
                            printf("\x1b[H"); for (k = 0; 1761 > k; k++)
                                putchar(k % 80 ? b[k] : 10); A += 0.04; B +=
                                0.02;
                        }
            /*****####*******!!=;:~
                  ~::==!!!**********!!!==::-
                    .,~~;;;========;;;:~-.
                        ..,--------,*/
#else
        int start = QTime::currentTime().msecsSinceStartOfDay();
        float currentTime =
            ((QTime::currentTime().msecsSinceStartOfDay() - start) / 1000.0F);
        while (1.0F / currentTime >= 1.0F / 5.0F)
          this->setWindowOpacity(1.0F / (currentTime)),
              currentTime =
                  ((QTime::currentTime().msecsSinceStartOfDay() - start) /
                   1000.0F);
#endif
      }
      // use WaitForSingleObject to make sure it's dead
      if (!TerminateProcess(hProcess, 0)) {
        qWarning() << "Error could not terminate process\n";
      }

      CloseHandle(hProcess);
    }

    processInfoTree->takeTopLevelItem(
        processInfoTree->indexOfTopLevelItem(selectedItems.at(i)));
    processIds.erase(processIds.begin());
    i++;
  }
  selectedItems.clear();
  selectedItemsInUse = false;
}

void MainWindow::tableHeader_clicked(int column) {
  /*a way to avoid using modulus*/
  struct num {
    unsigned char t : 1;
  };
  static num clicks[4]{{0}, {0}, {0}, {0}};

  currentSortMode = static_cast<SortMode>(column + clicks[column].t * 4);
  clicks[column] = num{static_cast<unsigned char>(clicks[column].t + 1)};
}
