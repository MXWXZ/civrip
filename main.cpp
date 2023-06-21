#include "civrip.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  CivRIP w;
  w.show();
  return a.exec();
}
