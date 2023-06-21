#include "civrip.h"
#include "ui_civrip.h"

#include <QCloseEvent>
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QSettings>

CivRIP::CivRIP(QWidget *parent) : QMainWindow(parent), ui(new Ui::CivRIP) {
  ui->setupUi(this);
  // config
  QSettings cfg("config.ini", QSettings::IniFormat);
  QString ip = cfg.value("/server/ip").toString();
  QString community = cfg.value("/server/community").toString();
  QString key = cfg.value("/server/key").toString();
  bool p2p = cfg.value("/server/p2p").toBool();
  bool tcp = cfg.value("/server/tcp").toBool();
  bool enc = cfg.value("/server/enc").toBool();
  auto lang = cfg.value("/server/lang").toString();
  ui->ip->setText(ip);
  ui->community->setText(community);
  ui->key->setText(key);
  ui->p2p->setChecked(p2p);
  ui->tcp->setChecked(tcp);
  ui->enc->setChecked(enc);

  // translator
  QDir dir("./i18n/");
  QStringList filter;
  filter << "*.qm";
  QFileInfoList flist =
      dir.entryInfoList(filter, QDir::NoDotAndDotDot | QDir::Files);
  ui->lang->addItem("en_US");
  ui->lang->setCurrentIndex(0);
  for (const auto &i : flist)
    ui->lang->addItem(i.fileName().left(i.fileName().length() - 3));

  translator = new QTranslator();
  bool flag = false;
  if (!lang.isEmpty() && lang != "en_US") {
    if (translator->load("./i18n/" + lang)) {
      qApp->installTranslator(translator);
      ui->lang->setCurrentText(lang);
      flag = true;
    }
  } else if (lang == "en_US") {
    flag = true;
  }
  if (!flag) {
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
      const QString baseName = QLocale(locale).name();
      if (translator->load("./i18n/" + baseName)) {
        qApp->installTranslator(translator);
        ui->lang->setCurrentText(baseName);
        break;
      }
    }
  }
  ui->retranslateUi(this);

  connect(&edge, SIGNAL(readyReadStandardOutput()), this, SLOT(writeOutLog()));
  connect(&edge, SIGNAL(readyReadStandardError()), this, SLOT(writeErrorLog()));
  connect(&edge, SIGNAL(finished(int, QProcess::ExitStatus)), this,
          SLOT(edgeFinished(int, QProcess::ExitStatus)));

  RefreshStatus();
}

CivRIP::~CivRIP() {
  delete ui;
  delete translator;
}

void CivRIP::ChangeTAP(STATUS_TAP status) {
  switch (status) {
  case STATUS_TAP_UNKNOWN:
    ui->status_tap->setStyleSheet("QLabel { color : red; }");
    ui->status_tap->setText(tr("UNKNOWN"));
    break;
  case STATUS_TAP_UNINSTALLED:
    ui->status_tap->setStyleSheet("QLabel { color : red; }");
    ui->status_tap->setText(tr("UNINST"));
    break;
  case STATUS_TAP_INSTALLED:
    ui->status_tap->setStyleSheet("QLabel { color : green; }");
    ui->status_tap->setText(tr("INST"));
    break;
  }
}

void CivRIP::ChangeWinIPB(STATUS_WINIPB status) {
  switch (status) {
  case STATUS_WINIPB_UNKNOWN:
    ui->status_winipb->setStyleSheet("QLabel { color : red; }");
    ui->status_winipb->setText(tr("UNKNOWN"));
    break;
  case STATUS_WINIPB_UNINSTALLED:
    ui->status_winipb->setStyleSheet("QLabel { color : red; }");
    ui->status_winipb->setText(tr("UNINST"));
    break;
  case STATUS_WINIPB_STOPPED:
    ui->status_winipb->setStyleSheet("QLabel { color : red; }");
    ui->status_winipb->setText(tr("STOPPED"));
    break;
  case STATUS_WINIPB_RUNNING:
    ui->status_winipb->setStyleSheet("QLabel { color : green; }");
    ui->status_winipb->setText(tr("RUNNING"));
    break;
  }
}

void CivRIP::RefreshStatus() {
  ui->output->clear();
  ui->output->append(QString("CivRIP Version %1").arg(VERSION));

  auto srvMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!srvMgr) {
    ui->output->append(QString("OpenSCManager failed: %1").arg(GetLastError()));
    ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
    return;
  }

  auto srvDDK = OpenService(srvMgr, _T("WinIPBroadcast"), SERVICE_ALL_ACCESS);
  if (srvDDK) {
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;
    if (QueryServiceStatusEx(srvDDK, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
                             sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
      if (ssStatus.dwCurrentState == SERVICE_RUNNING)
        ChangeWinIPB(STATUS_WINIPB_RUNNING);
      else
        ChangeWinIPB(STATUS_WINIPB_STOPPED);
    } else {
      ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
    }
  } else {
    ChangeWinIPB(STATUS_WINIPB_UNINSTALLED);
  }

  if (srvMgr)
    CloseServiceHandle(srvMgr);
  if (srvDDK)
    CloseServiceHandle(srvDDK);

  // tap
  auto ret = GetTAPDevice();
  if (ret == -2)
    ChangeTAP(STATUS_TAP_UNKNOWN);
  else if (ret == -1)
    ChangeTAP(STATUS_TAP_UNINSTALLED);
  else
    ChangeTAP(STATUS_TAP_INSTALLED);
}

BOOL CivRIP::QueryService(SC_HANDLE mgr, SC_HANDLE ddk,
                          SERVICE_STATUS_PROCESS *status) {
  DWORD dwBytesNeeded;
  if (!QueryServiceStatusEx(ddk, SC_STATUS_PROCESS_INFO, (LPBYTE)status,
                            sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
    ui->output->append(
        QString("QueryServiceStatusEx failed: %1").arg(GetLastError()));
    CloseServiceHandle(mgr);
    CloseServiceHandle(ddk);
    return FALSE;
  }
  return TRUE;
}

void CivRIP::on_pushButton_clicked() {
  auto srvMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!srvMgr) {
    ui->output->append(QString("OpenSCManager failed: %1").arg(GetLastError()));
    ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
    return;
  }

  auto srvDDK = OpenService(srvMgr, _T("WinIPBroadcast"), SERVICE_ALL_ACCESS);
  if (srvDDK) {
    SERVICE_STATUS_PROCESS ssStatus;
    if (!QueryService(srvMgr, srvDDK, &ssStatus)) {
      ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
      return;
    }

    ControlService(srvDDK, SERVICE_CONTROL_STOP,
                   (LPSERVICE_STATUS)&ssStatus); // ignore stop error
    while (ssStatus.dwCurrentState != SERVICE_STOPPED) {
      auto dwWaitTime = ssStatus.dwWaitHint / 10;
      if (dwWaitTime < 1000)
        dwWaitTime = 1000;
      else if (dwWaitTime > 10000)
        dwWaitTime = 10000;
      Sleep(dwWaitTime);

      if (!QueryService(srvMgr, srvDDK, &ssStatus)) {
        ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
        return;
      }
      if (ssStatus.dwCurrentState == SERVICE_STOPPED)
        break;
    }
    ui->output->append(QString("WinIPBroadcast Stopped"));
    ChangeWinIPB(STATUS_WINIPB_STOPPED);
    repaint();
    Sleep(1000);

    if (!StartService(srvDDK, NULL, NULL)) {
      ui->output->append(
          QString("StartService failed: %1").arg(GetLastError()));
    } else {
      if (!QueryService(srvMgr, srvDDK, &ssStatus)) {
        ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
        return;
      }
      auto dwStartTickCount = GetTickCount();
      auto dwOldCheckPoint = ssStatus.dwCheckPoint;
      while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {
        auto dwWaitTime = ssStatus.dwWaitHint / 10;
        if (dwWaitTime < 1000)
          dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
          dwWaitTime = 10000;
        Sleep(dwWaitTime);

        if (!QueryService(srvMgr, srvDDK, &ssStatus)) {
          ChangeWinIPB(STATUS_WINIPB_UNKNOWN);
          return;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
          dwStartTickCount = GetTickCount();
          dwOldCheckPoint = ssStatus.dwCheckPoint;
        } else {
          if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint) {
            break;
          }
        }
      }

      if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        ui->output->append(QString("WinIPBroadcast Started"));
        ChangeWinIPB(STATUS_WINIPB_RUNNING);
      } else {
        ui->output->append(
            QString("StartService failed: %1").arg(GetLastError()));
      }
    }
  } else {
    ChangeWinIPB(STATUS_WINIPB_UNINSTALLED);
  }

  if (srvMgr)
    CloseServiceHandle(srvMgr);
  if (srvDDK)
    CloseServiceHandle(srvDDK);
}

int CivRIP::GetTAPDevice() {
  int ret = -2;
  HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
  hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT);
  if (INVALID_HANDLE_VALUE == hDevInfo) {
    ui->output->append(
        QString("SetupDiGetClassDevs failed: %1").arg(GetLastError()));
    return ret;
  }

  ret = -1;
  SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};
  for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
    LPOLESTR guidtemp;
    StringFromCLSID(DeviceInfoData.ClassGuid, &guidtemp);
    auto guid = QString::fromWCharArray(guidtemp).trimmed();
    CoTaskMemFree(guidtemp);
    wchar_t insidtemp[128];
    if (guid.compare("{4d36e972-e325-11ce-bfc1-08002be10318}",
                     Qt::CaseInsensitive) == 0) {
      SetupDiGetDeviceInstanceId(hDevInfo, &DeviceInfoData, insidtemp, 128,
                                 NULL);
      auto insid = QString::fromWCharArray(insidtemp).trimmed();
      if (insid.startsWith("ROOT\\NET\\", Qt::CaseInsensitive)) {
        ret = i;
        break;
      }
    }
  }
  SetupDiDestroyDeviceInfoList(hDevInfo);
  return ret;
}

void CivRIP::on_pushButton_2_clicked() {
  auto dev = GetTAPDevice();
  if (dev == -2) {
    ChangeTAP(STATUS_TAP_UNKNOWN);
  } else if (dev == -1) {
    ChangeTAP(STATUS_TAP_UNINSTALLED);
  } else {
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    hDevInfo =
        SetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL, DIGCF_PRESENT);
    if (INVALID_HANDLE_VALUE == hDevInfo) {
      ui->output->append(
          QString("SetupDiGetClassDevs failed: %1").arg(GetLastError()));
      ChangeTAP(STATUS_TAP_UNKNOWN);
      return;
    }
    SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};
    if (!SetupDiEnumDeviceInfo(hDevInfo, dev, &DeviceInfoData)) {
      ChangeTAP(STATUS_TAP_UNINSTALLED);
    } else {
      SP_PROPCHANGE_PARAMS params = {sizeof(SP_CLASSINSTALL_HEADER)};
      params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
      params.Scope = DICS_FLAG_CONFIGSPECIFIC;
      params.StateChange = DICS_DISABLE;
      params.HwProfile = 0;
      SetupDiSetClassInstallParams(hDevInfo, &DeviceInfoData,
                                   (SP_CLASSINSTALL_HEADER *)&params,
                                   sizeof(SP_PROPCHANGE_PARAMS));
      SetupDiChangeState(hDevInfo, &DeviceInfoData);
      ui->output->append(QString("TAP Windows Disabled"));
      ChangeTAP(STATUS_TAP_UNINSTALLED);
      repaint();
      Sleep(1000);

      params.StateChange = DICS_ENABLE;
      SetupDiSetClassInstallParams(hDevInfo, &DeviceInfoData,
                                   (SP_CLASSINSTALL_HEADER *)&params,
                                   sizeof(SP_PROPCHANGE_PARAMS));
      SetupDiChangeState(hDevInfo, &DeviceInfoData);
      ui->output->append(QString("TAP Windows Enabled"));
      ChangeTAP(STATUS_TAP_INSTALLED);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
  }
}

void CivRIP::on_pushButton_4_clicked() { RefreshStatus(); }

void CivRIP::on_pushButton_5_clicked() {
  QProcess process;
  process.startCommand("WinIPBroadcast-1.6.exe /VERYSILENT");
  process.waitForFinished(-1);
  RefreshStatus();
  ui->output->append(
      QString("WinIPBroadcast Installed, return: %1").arg(process.exitCode()));
}

void CivRIP::on_lang_currentIndexChanged(int index) {
  auto sel = ui->lang->itemText(index);
  qApp->removeTranslator(translator);
  if (sel != "en_US" && translator->load("./i18n/" + sel))
    qApp->installTranslator(translator);
  ui->retranslateUi(this);
  QSettings cfg("config.ini", QSettings::IniFormat);
  cfg.setValue("/server/lang", sel);
  RefreshStatus();
}

void CivRIP::on_pushButton_6_clicked() {
  QProcess process;
  process.startCommand("tap-windows-9.21.2.exe /S");
  process.waitForFinished(-1);
  RefreshStatus();
  ui->output->append(
      QString("TAP Windows Installed, return: %1").arg(process.exitCode()));
}

void CivRIP::on_connect_clicked() {
  if (ui->connect->text() == tr("Connect")) {
    ui->connect->setText(tr("Disconnect"));
    ui->status_connect->setStyleSheet("QLabel { color : orange; }");
    ui->status_connect->setText(tr("TRYING"));

    QSettings cfg("config.ini", QSettings::IniFormat);
    cfg.setValue("/server/ip", ui->ip->text());
    cfg.setValue("/server/community", ui->community->text());
    cfg.setValue("/server/key", ui->key->text());
    cfg.setValue("/server/p2p", ui->p2p->isChecked());
    cfg.setValue("/server/tcp", ui->tcp->isChecked());
    cfg.setValue("/server/enc", ui->enc->isChecked());

    auto cmd = QString("edge.exe -c %1 -l %2 -m ")
                   .arg(ui->community->text(), ui->ip->text());

    QString mac;
    char tmp[16];
    for (auto i = 0; i < 6; i++) {
      auto tp = QRandomGenerator::global()->bounded(256);
      snprintf(tmp, 16, "%s%X%s", tp < 16 ? "0" : "", tp, i < 5 ? ":" : "");
      mac.append(tmp);
    }
    cmd += mac;

    if (ui->p2p->isChecked()) {
      if (ui->tcp->isChecked())
        cmd += " -S2";
      else
        cmd += " -S1";
    }
    if (!ui->enc->isChecked()) {
      cmd += QString(" -z2 -k %1").arg(ui->key->text());
    }
    ui->output->append(cmd);
    edge.startCommand(cmd);
  } else {
    edge.terminate();
    ui->connect->setText(tr("Waiting"));
    repaint();
    Sleep(3000);
    edge.kill();
    edge.close();
  }
}

void CivRIP::edgeFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  ui->output->append(QString("edge exit code: %1").arg(exitCode));
  ui->status_connect->setStyleSheet("QLabel { color : red; }");
  ui->status_connect->setText(tr("DISCONN"));
  ui->connect->setText(tr("Connect"));
}

void CivRIP::writeOutLog() {
  ui->output->append(edge.readAllStandardOutput());
  auto out = ui->output->toPlainText().split("\n");
  for (int i = out.length() - 1; i >= 0 && out.length() - i < 5; i--) {
    if (out[i].contains("[OK] edge")) {
      ui->status_connect->setStyleSheet("QLabel { color : green; }");
      ui->status_connect->setText(tr("CONNECT"));
      break;
    } else if (out[i].contains("ERROR")) {
      ui->status_connect->setStyleSheet("QLabel { color : red; }");
      ui->status_connect->setText(tr("ERROR"));
      break;
    }
  }
}

void CivRIP::writeErrorLog() {
  ui->output->append("[ERROR] " + edge.readAllStandardError());
}

void CivRIP::on_output_textChanged() {
  ui->output->verticalScrollBar()->setValue(
      ui->output->verticalScrollBar()->maximum());
}

void CivRIP::closeEvent(QCloseEvent *event) {
  event->ignore();
  if (ui->status_connect->text() == tr("DISCONN")) {
    event->accept();
  } else if (QMessageBox::Yes ==
             QMessageBox::warning(
                 this, tr("Close Confirmation"),
                 tr("Close application will disconnect from server, confirm?"),
                 QMessageBox::Yes | QMessageBox::No)) {
    event->accept();
  }
}
