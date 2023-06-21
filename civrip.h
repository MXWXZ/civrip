#ifndef CIVRIP_H
#define CIVRIP_H

#define VERSION "1.0.0"

#include <QMainWindow>
#include <QProcess>
#include <QTranslator>
#include <tchar.h>
#include <windows.h>

#include <initguid.h>

#include <devguid.h>
#include <setupapi.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class CivRIP;
}
QT_END_NAMESPACE

enum STATUS_WINIPB {
  STATUS_WINIPB_UNKNOWN,
  STATUS_WINIPB_UNINSTALLED,
  STATUS_WINIPB_STOPPED,
  STATUS_WINIPB_RUNNING,
};

enum STATUS_TAP {
  STATUS_TAP_UNKNOWN,
  STATUS_TAP_UNINSTALLED,
  STATUS_TAP_INSTALLED,
};

class CivRIP : public QMainWindow {
  Q_OBJECT

public:
  CivRIP(QWidget *parent = nullptr);
  ~CivRIP();

private slots:
  void on_pushButton_clicked();
  BOOL QueryService(SC_HANDLE mgr, SC_HANDLE ddk,
                    SERVICE_STATUS_PROCESS *status);
  int GetTAPDevice();
  void RefreshStatus();
  void ChangeWinIPB(STATUS_WINIPB status);
  void ChangeTAP(STATUS_TAP status);

  void on_pushButton_2_clicked();

  void on_pushButton_4_clicked();

  void on_pushButton_5_clicked();

  void on_lang_currentIndexChanged(int index);

  void on_pushButton_6_clicked();

  void on_connect_clicked();

  void writeOutLog();
  void writeErrorLog();
  void edgeFinished(int exitCode, QProcess::ExitStatus exitStatus);

  void on_output_textChanged();

  void closeEvent(QCloseEvent *event) override;

private:
  QProcess edge;
  QTranslator *translator;
  Ui::CivRIP *ui;
};
#endif // CIVRIP_H
