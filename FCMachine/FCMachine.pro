QT += core gui widgets serialport xml svg concurrent
CONFIG += c++17 warn_on

GENERATED_PRI = $$PWD/FCMachine.pri

!exists($$GENERATED_PRI)|!isEmpty(REGENERATE_PRI) {
    message("Генерация конфигурации: $$GENERATED_PRI")
    system("$$PWD/generate.pri.sh")
}

include($$GENERATED_PRI)

QMAKE_CFLAGS   += --sysroot=$$[QT_SYSROOT]
QMAKE_CXXFLAGS += --sysroot=$$[QT_SYSROOT]
QMAKE_LFLAGS   += --sysroot=$$[QT_SYSROOT]

PKGCONFIG += gpiod

;SOURCES += $$PWD/main.cpp

# Заголовочные файлы (опционально — для навигации в Qt Creator)
;HEADERS += $$files($$PWD/*.h)

TARGET = FCMachine
TEMPLATE = app

unix:!android {
    target.path = /root/$${TARGET}
    INSTALLS += target
}

contains(CONFIG, debug) {
    QMAKE_CXXFLAGS += -g
}
