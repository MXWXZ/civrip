QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    civrip.cpp

HEADERS += \
    civrip.h

FORMS += \
    civrip.ui

CONFIG += lrelease

RC_FILE = main.rc
LIBS += -lsetupapi -lole32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    main.rc \
    uac.manifest

copydata.commands = $(COPY_DIR) $$shell_path($$PWD/i18n//*.qm) $$shell_path($$OUT_PWD/i18n/)
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata
