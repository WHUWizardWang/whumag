QT       += core gui quickwidgets location positioning sql printsupport network quick qml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    3DView/glwidget.cpp \
    3DView/omgglctrl.cpp \
    3DView/omgglwidget.cpp \
    3DView/terrain.cpp \
    MagAno/anoqueryform.cpp \
    MagAno/anoqueryfromgridsetform.cpp \
    MagAno/anoqueryfrompointsetform.cpp \
    Map/mapform.cpp \
    Map/omggeopoint.cpp \
    Map/omgpolygon.cpp \
    Map/omgqmlpoint.cpp \
    Map/omgqmlpolygon.cpp \
    Map/omgraster.cpp \
    ReadData.cpp \
    buildprojectform.cpp \
    contourplotter.cpp \
    database/database.cpp \
    dataprocessing/MagneticComplexityAnalyzer.cpp \
    dataprocessing/Time_TongHua.cpp \
    dataprocessing/accuracy.cpp \
    dataprocessing/extension.cpp \
    dataprocessing/fftw.cpp \
    dataprocessing/inputpara_dp.cpp \
    dataprocessing/merge.cpp \
    dataprocessing/mergeform.cpp \
    dataprocessing/readFile.cpp \
    draw/draw_form.cpp \
    draw/qcustomplot.cpp \
    geomagnetismproject.cpp \
    help.cpp \
    importform.cpp \
    main.cpp \
    mainwindow.cpp \
    myopenglwidget.cpp \
    navigation/autonav.cpp \
    navigation/function.cpp \
    navigation/iccp.cpp \
    navigation/inputpathform.cpp \
    navigation/sitan.cpp \
    navigation/tercom.cpp \
    project.cpp \
    projectmanager.cpp \
    referencemap/CompressiveSensing.cpp \
    referencemap/GeomagneticModel.cpp \
    referencemap/LSSVMPSO.cpp \
    referencemap/ReconstructionManager.cpp \
    referencemap/globalmodel/autoreferencemap.cpp \
    referencemap/globalmodel/mapTaylorLegendreform.cpp \
    referencemap/globalmodel/mapcompressform.cpp \
    referencemap/globalmodel/maplssvmpsoform.cpp \
    referencemap/globalmodel/mappolyhedralform.cpp \
    referencemap/globalmodel/mapsplineform.cpp \
    referencemap/globalmodel/referencemap.cpp \
    referencemap/inputpara_rm.cpp \
    subarea/subarea.cpp \
    subarea/subareablocks.cpp \
    utils.cpp \
    wmm/GeomagInteractiveLib.c \
    wmm/GeomagnetismLibrary.c \
    wmm/igrf_point.c \
    wmm/magcalc.c \
    wmm/queryform.cpp \
    wmm/queryfromfilesetform.cpp \
    wmm/queryfromgridsetform.cpp \
    wmm/queryfrompointsetform.cpp \
    wmm/wmm_point.c

HEADERS += \
    3DView/glwidget.h \
    3DView/omgglctrl.h \
    3DView/omgglwidget.h \
    3DView/terrain.h \
    DataStruct.h \
    MagAno/OmgValidator.h \
    MagAno/anoqueryform.h \
    MagAno/anoqueryfromgridsetform.h \
    MagAno/anoqueryfrompointsetform.h \
    MagAno/maganoquery.h \
    Map/mapform.h \
    Map/omggeopoint.h \
    Map/omgpolygon.h \
    Map/omgqmlpoint.h \
    Map/omgqmlpolygon.h \
    Map/omgraster.h \
    ReadData.h \
    buildprojectform.h \
    contourplotter.h \
    database/database.h \
    database/databasemanager.h \
    dataprocessing/MagneticComplexityAnalyzer.h \
    dataprocessing/Time_TongHua.h \
    dataprocessing/accuracy.h \
    dataprocessing/extension.h \
    dataprocessing/fftw.h \
    dataprocessing/inputpara_dp.h \
    dataprocessing/merge.h \
    dataprocessing/mergeform.h \
    dataprocessing/readFile.h \
    draw/draw_form.h \
    draw/qcustomplot.h \
    eigenqdebug.h \
    geomagnetismproject.h \
    gmparameters.h \
    help.h \
    importform.h \
    mainwindow.h \
    myopenglwidget.h \
    navigation/autonav.h \
    navigation/function.h \
    navigation/iccp.h \
    navigation/inputpathform.h \
    navigation/sitan.h \
    navigation/tercom.h \
    project.h \
    projectmanager.h \
    realtime_redirector.h \
    referencemap/CompressiveSensing.h \
    referencemap/GeomagneticModel.h \
    referencemap/LSSVMPSO.h \
    referencemap/ReconstructionManager.h \
    referencemap/globalmodel/autoreferencemap.h \
    referencemap/globalmodel/mapTaylorLegendreform.h \
    referencemap/globalmodel/mapcompressform.h \
    referencemap/globalmodel/maplssvmpsoform.h \
    referencemap/globalmodel/mappolyhedralform.h \
    referencemap/globalmodel/mapsplineform.h \
    referencemap/globalmodel/referencemap.h \
    referencemap/inputpara_rm.h \
    subarea/subarea.h \
    subarea/subareablocks.h \
    utils.h \
    wmm/EGM9615.h \
    wmm/GeomagInterativeLib.h \
    wmm/GeomagnetismHeader.h \
    wmm/OmgValidator.h \
    wmm/igrf_point.h \
    wmm/magcalc.h \
    wmm/queryform.h \
    wmm/queryfromfilesetform.h \
    wmm/queryfromgridsetform.h \
    wmm/queryfrompointsetform.h \
    wmm/version.h \
    wmm/wmm_point.h

FORMS += \
    3DView/omgglctrl.ui \
    3DView/omgglwidget.ui \
    MagAno/anoqueryform.ui \
    MagAno/anoqueryfromgridsetform.ui \
    MagAno/anoqueryfrompointsetform.ui \
    Map/mapform.ui \
    buildprojectform.ui \
    database/database.ui \
    dataprocessing/mergeform.ui \
    draw/draw_form.ui \
    help.ui \
    importform.ui \
    mainwindow.ui \
    navigation/inputpathform.ui \
    referencemap/globalmodel/autoreferencemap.ui \
    referencemap/globalmodel/mapTaylorLegendreform.ui \
    referencemap/globalmodel/mapcompressform.ui \
    referencemap/globalmodel/maplssvmpsoform.ui \
    referencemap/globalmodel/mappolyhedralform.ui \
    referencemap/globalmodel/mapsplineform.ui \
    referencemap/globalmodel/referencemap.ui \
    wmm/queryform.ui \
    wmm/queryfromfilesetform.ui \
    wmm/queryfromgridsetform.ui \
    wmm/queryfrompointsetform.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    cs.py \
    high_quality/high_quality.rcc \
    plot.py \
    wmm/COF/EMM2000.COF \
    wmm/COF/EMM2000SV.COF \
    wmm/COF/EMM2001.COF \
    wmm/COF/EMM2001SV.COF \
    wmm/COF/EMM2002.COF \
    wmm/COF/EMM2002SV.COF \
    wmm/COF/EMM2003.COF \
    wmm/COF/EMM2003SV.COF \
    wmm/COF/EMM2004.COF \
    wmm/COF/EMM2004SV.COF \
    wmm/COF/EMM2005.COF \
    wmm/COF/EMM2005SV.COF \
    wmm/COF/EMM2006.COF \
    wmm/COF/EMM2006SV.COF \
    wmm/COF/EMM2007.COF \
    wmm/COF/EMM2007SV.COF \
    wmm/COF/EMM2008.COF \
    wmm/COF/EMM2008SV.COF \
    wmm/COF/EMM2009.COF \
    wmm/COF/EMM2009SV.COF \
    wmm/COF/EMM2010.COF \
    wmm/COF/EMM2010SV.COF \
    wmm/COF/EMM2011.COF \
    wmm/COF/EMM2011SV.COF \
    wmm/COF/EMM2012.COF \
    wmm/COF/EMM2012SV.COF \
    wmm/COF/EMM2013.COF \
    wmm/COF/EMM2013SV.COF \
    wmm/COF/EMM2014.COF \
    wmm/COF/EMM2014SV.COF \
    wmm/COF/EMM2015.COF \
    wmm/COF/EMM2015SV.COF \
    wmm/COF/EMM2016.COF \
    wmm/COF/EMM2016SV.COF \
    wmm/COF/EMM2017.COF \
    wmm/COF/EMM2017SV.COF \
    wmm/COF/IGRF13.COF \
    wmm/COF/WMM.COF

RESOURCES += \
    cof.qrc \
    icons.qrc \
    qml.qrc
DEFINES += BYTE_DEFINED
CONFIG += openssl-linked  # 静态链接 OpenSSL
# INCLUDEPATH += /usr/local/lib \
#                /usr/include/python3.8 \
#                /usr/local/fftw_arm/include
#                /usr/local/fftw_arm/lib

LIBS += -L$$PWD/ -llibfftw3-3
# QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_CXXFLAGS += -Os
QMAKE_CXXFLAGS += -fdata-sections -ffunction-sections
QMAKE_LFLAGS += -Wl,--gc-sections
# win32 {
#     # Windows平台
#     QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$OUT_PWD\\debug\\resources\\) mkdir $$shell_path($$OUT_PWD\\debug\\resources\\))
#     QMAKE_POST_LINK += $$quote(if not exist $$shell_path($$OUT_PWD\\release\\resources\\) mkdir $$shell_path($$OUT_PWD\\release\\resources\\))
#     CONFIG(debug, debug|release) {
#         QMAKE_POST_LINK += $$quote(copy /y $$shell_path($$PWD\\high_quality.rcc) $$shell_path($$OUT_PWD\\debug\\resources\\))
#     } else {
#         QMAKE_POST_LINK += $$quote(copy /y $$shell_path($$PWD\\high_quality.rcc) $$shell_path($$OUT_PWD\\release\\resources\\))
#     }
# }

# unix {
#     # Linux/macOS平台
#     QMAKE_POST_LINK += $$quote(mkdir -p $$OUT_PWD/resources/ && cp -f $$PWD/high_quality.rcc $$OUT_PWD/resources/)
# }

