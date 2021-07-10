QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

QMAKE_CXXFLAGS_RELEASE += -O3

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Mapper/mapper_0.cpp \
    Mapper/mapper_1.cpp \
    Mapper/mapper_2.cpp \
    Mapper/mapper_3.cpp \
    Mapper/mapper_4.cpp \
    Mapper/mapper_66.cpp \
    Simple_Apu.cpp \
    bus.cpp \
    cartridge.cpp \
    controller.cpp \
    cpu.cpp \
    debugger.cpp \
    main.cpp \
    mainwindow.cpp \
    nes_apu/Blip_Buffer.cpp \
    nes_apu/Multi_Buffer.cpp \
    nes_apu/Nes_Apu.cpp \
    nes_apu/Nes_Namco.cpp \
    nes_apu/Nes_Oscs.cpp \
    nes_apu/Nes_Vrc6.cpp \
    nes_apu/Nonlinear_Buffer.cpp \
    nes_apu/apu_snapshot.cpp \
    ppu.cpp

HEADERS += \
    Mapper/mapper.h \
    Mapper/mapper_0.h \
    Mapper/mapper_1.h \
    Mapper/mapper_2.h \
    Mapper/mapper_3.h \
    Mapper/mapper_4.h \
    Mapper/mapper_66.h \
    Simple_Apu.h \
    boost/config.hpp \
    boost/cstdint.hpp \
    boost/static_assert.hpp \
    bus.h \
    cartridge.h \
    component.h \
    controller.h \
    cpu.h \
    debugger.h \
    mainwindow.h \
    nes_apu/Blip_Buffer.h \
    nes_apu/Blip_Synth.h \
    nes_apu/Multi_Buffer.h \
    nes_apu/Nes_Apu.h \
    nes_apu/Nes_Namco.h \
    nes_apu/Nes_Oscs.h \
    nes_apu/Nes_Vrc6.h \
    nes_apu/Nonlinear_Buffer.h \
    nes_apu/apu_snapshot.h \
    nes_apu/blargg_common.h \
    nes_apu/blargg_source.h \
    palette.h \
    ppu.h

FORMS += \
    debugger.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
