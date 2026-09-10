QT       += core gui quickwidgets location positioning sql printsupport network quick qml concurrent

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

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/app \
    $$PWD/third_party \
    $$PWD/third_party/qcustomplot

SOURCES += \
    src/3DView/glwidget.cpp \
    src/3DView/omgglctrl.cpp \
    src/3DView/omgglwidget.cpp \
    src/3DView/terrain.cpp \
    src/MagAno/anoqueryform.cpp \
    src/MagAno/anoqueryfromgridsetform.cpp \
    src/MagAno/anoqueryfrompointsetform.cpp \
    src/Map/mapform.cpp \
    src/Map/omggeopoint.cpp \
    src/Map/omgpolygon.cpp \
    src/Map/omgqmlpoint.cpp \
    src/Map/omgqmlpolygon.cpp \
    src/Map/omgraster.cpp \
    src/app/ReadData.cpp \
    third_party/alglib/alglibinternal.cpp \
    third_party/alglib/alglibmisc.cpp \
    third_party/alglib/ap.cpp \
    third_party/alglib/dataanalysis.cpp \
    third_party/alglib/diffequations.cpp \
    third_party/alglib/fasttransforms.cpp \
    third_party/alglib/integration.cpp \
    third_party/alglib/interpolation.cpp \
    third_party/alglib/kernels_avx2.cpp \
    third_party/alglib/kernels_fma.cpp \
    third_party/alglib/kernels_sse2.cpp \
    third_party/alglib/linalg.cpp \
    third_party/alglib/optimization.cpp \
    third_party/alglib/solvers.cpp \
    third_party/alglib/specialfunctions.cpp \
    third_party/alglib/statistics.cpp \
    third_party/qcustomplot/qcustomplot.cpp \
    src/app/buildprojectform.cpp \
    src/app/contourplotter.cpp \
    src/database/database.cpp \
    src/dataprocessing/MagneticComplexityAnalyzer.cpp \
    src/dataprocessing/Time_TongHua.cpp \
    src/dataprocessing/accuracy.cpp \
    src/dataprocessing/extension.cpp \
    src/dataprocessing/inputpara_dp.cpp \
    src/dataprocessing/merge.cpp \
    src/dataprocessing/mergeform.cpp \
    src/draw/draw_form.cpp \
    src/app/geomagnetismproject.cpp \
    src/app/help.cpp \
    src/app/importform.cpp \
    src/app/main.cpp \
    src/app/mainwindow.cpp \
    src/app/myopenglwidget.cpp \
    src/navigation/autonav.cpp \
    src/navigation/function.cpp \
    src/navigation/iccp.cpp \
    src/navigation/inputpathform.cpp \
    src/navigation/sitan.cpp \
    src/navigation/tercom.cpp \
    src/app/project.cpp \
    src/app/projectmanager.cpp \
    src/referencemap/CompressiveSensing.cpp \
    src/referencemap/GeomagneticModel.cpp \
    src/referencemap/LSSVMPSO.cpp \
    src/referencemap/ReconstructionManager.cpp \
    src/referencemap/globalmodel/autoreferencemap.cpp \
    src/referencemap/globalmodel/mapTaylorLegendreform.cpp \
    src/referencemap/globalmodel/mapcompressform.cpp \
    src/referencemap/globalmodel/maplssvmpsoform.cpp \
    src/referencemap/globalmodel/mappolyhedralform.cpp \
    src/referencemap/globalmodel/mapsplineform.cpp \
    src/referencemap/globalmodel/referencemap.cpp \
    src/referencemap/inputpara_rm.cpp \
    src/subarea/subareablocks.cpp \
    src/app/utils.cpp \
    src/wmm/GeomagnetismLibrary.c \
    src/wmm/igrf_point.c \
    src/wmm/magcalc.c \
    src/wmm/queryform.cpp \
    src/wmm/queryfromfilesetform.cpp \
    src/wmm/queryfromgridsetform.cpp \
    src/wmm/queryfrompointsetform.cpp \
    src/wmm/wmm_point.c

HEADERS += \
    src/3DView/glwidget.h \
    src/3DView/omgglctrl.h \
    src/3DView/omgglwidget.h \
    src/3DView/terrain.h \
    src/app/DataStruct.h \
    src/MagAno/OmgValidator.h \
    src/MagAno/anoqueryform.h \
    src/MagAno/anoqueryfromgridsetform.h \
    src/MagAno/anoqueryfrompointsetform.h \
    src/MagAno/maganoquery.h \
    src/Map/mapform.h \
    src/Map/omggeopoint.h \
    src/Map/omgpolygon.h \
    src/Map/omgqmlpoint.h \
    src/Map/omgqmlpolygon.h \
    src/Map/omgraster.h \
    src/app/ReadData.h \
    third_party/alglib/alglibinternal.h \
    third_party/alglib/alglibmisc.h \
    third_party/alglib/ap.h \
    third_party/alglib/dataanalysis.h \
    third_party/alglib/diffequations.h \
    third_party/alglib/fasttransforms.h \
    third_party/alglib/integration.h \
    third_party/alglib/interpolation.h \
    third_party/alglib/kernels_avx2.h \
    third_party/alglib/kernels_fma.h \
    third_party/alglib/kernels_sse2.h \
    third_party/alglib/linalg.h \
    third_party/alglib/optimization.h \
    third_party/alglib/solvers.h \
    third_party/alglib/specialfunctions.h \
    third_party/alglib/statistics.h \
    third_party/alglib/stdafx.h \
    src/app/buildprojectform.h \
    src/app/contourplotter.h \
    src/database/database.h \
    src/database/databasemanager.h \
    src/dataprocessing/MagneticComplexityAnalyzer.h \
    src/dataprocessing/Time_TongHua.h \
    src/dataprocessing/accuracy.h \
    src/dataprocessing/extension.h \
    src/dataprocessing/inputpara_dp.h \
    src/dataprocessing/merge.h \
    src/dataprocessing/mergeform.h \
    src/draw/draw_form.h \
    third_party/qcustomplot/qcustomplot.h \
    src/app/eigenqdebug.h \
    src/app/geomagnetismproject.h \
    src/app/gmparameters.h \
    src/app/help.h \
    src/app/importform.h \
    src/app/mainwindow.h \
    src/app/myopenglwidget.h \
    third_party/nanoflann.hpp \
    src/navigation/autonav.h \
    src/navigation/function.h \
    src/navigation/iccp.h \
    src/navigation/inputpathform.h \
    src/navigation/sitan.h \
    src/navigation/tercom.h \
    src/app/project.h \
    src/app/projectmanager.h \
    src/app/realtime_redirector.h \
    src/referencemap/CompressiveSensing.h \
    src/referencemap/GeomagneticModel.h \
    src/referencemap/KDTree.h \
    src/referencemap/LSSVMPSO.h \
    src/referencemap/ReconstructionManager.h \
    src/referencemap/globalmodel/autoreferencemap.h \
    src/referencemap/globalmodel/mapTaylorLegendreform.h \
    src/referencemap/globalmodel/mapcompressform.h \
    src/referencemap/globalmodel/maplssvmpsoform.h \
    src/referencemap/globalmodel/mappolyhedralform.h \
    src/referencemap/globalmodel/mapsplineform.h \
    src/referencemap/globalmodel/referencemap.h \
    src/referencemap/inputpara_rm.h \
    src/referencemap/stable.h \
    src/subarea/subareablocks.h \
    src/app/utils.h \
    src/wmm/EGM9615.h \
    src/wmm/GeomagnetismHeader.h \
    src/wmm/OmgValidator.h \
    src/wmm/igrf_point.h \
    src/wmm/magcalc.h \
    src/wmm/queryform.h \
    src/wmm/queryfromfilesetform.h \
    src/wmm/queryfromgridsetform.h \
    src/wmm/queryfrompointsetform.h \
    src/wmm/version.h \
    src/wmm/wmm_point.h

FORMS += \
    src/3DView/omgglctrl.ui \
    src/3DView/omgglwidget.ui \
    src/MagAno/anoqueryform.ui \
    src/MagAno/anoqueryfromgridsetform.ui \
    src/MagAno/anoqueryfrompointsetform.ui \
    src/Map/mapform.ui \
    src/app/buildprojectform.ui \
    src/database/database.ui \
    src/dataprocessing/mergeform.ui \
    src/draw/draw_form.ui \
    src/app/help.ui \
    src/app/importform.ui \
    src/app/mainwindow.ui \
    src/navigation/inputpathform.ui \
    src/referencemap/globalmodel/autoreferencemap.ui \
    src/referencemap/globalmodel/mapTaylorLegendreform.ui \
    src/referencemap/globalmodel/mapcompressform.ui \
    src/referencemap/globalmodel/maplssvmpsoform.ui \
    src/referencemap/globalmodel/mappolyhedralform.ui \
    src/referencemap/globalmodel/mapsplineform.ui \
    src/referencemap/globalmodel/referencemap.ui \
    src/wmm/queryform.ui \
    src/wmm/queryfromfilesetform.ui \
    src/wmm/queryfromgridsetform.ui \
    src/wmm/queryfrompointsetform.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    resources/high_quality.rcc \
    resources/COF/EMM2000.COF \
    resources/COF/EMM2000SV.COF \
    resources/COF/EMM2001.COF \
    resources/COF/EMM2001SV.COF \
    resources/COF/EMM2002.COF \
    resources/COF/EMM2002SV.COF \
    resources/COF/EMM2003.COF \
    resources/COF/EMM2003SV.COF \
    resources/COF/EMM2004.COF \
    resources/COF/EMM2004SV.COF \
    resources/COF/EMM2005.COF \
    resources/COF/EMM2005SV.COF \
    resources/COF/EMM2006.COF \
    resources/COF/EMM2006SV.COF \
    resources/COF/EMM2007.COF \
    resources/COF/EMM2007SV.COF \
    resources/COF/EMM2008.COF \
    resources/COF/EMM2008SV.COF \
    resources/COF/EMM2009.COF \
    resources/COF/EMM2009SV.COF \
    resources/COF/EMM2010.COF \
    resources/COF/EMM2010SV.COF \
    resources/COF/EMM2011.COF \
    resources/COF/EMM2011SV.COF \
    resources/COF/EMM2012.COF \
    resources/COF/EMM2012SV.COF \
    resources/COF/EMM2013.COF \
    resources/COF/EMM2013SV.COF \
    resources/COF/EMM2014.COF \
    resources/COF/EMM2014SV.COF \
    resources/COF/EMM2015.COF \
    resources/COF/EMM2015SV.COF \
    resources/COF/EMM2016.COF \
    resources/COF/EMM2016SV.COF \
    resources/COF/EMM2017.COF \
    resources/COF/EMM2017SV.COF \
    resources/COF/IGRF13.COF \
    resources/COF/IGRF14_Windows.COF \
    resources/COF/WMM.COF

RESOURCES += \
    resources/cof.qrc \
    resources/icons.qrc \
    resources/qml.qrc
DEFINES += BYTE_DEFINED
CONFIG += openssl-linked  # 静态链接 OpenSSL
# INCLUDEPATH += /usr/local/lib \
#                /usr/include/python3.8 \
#                /usr/local/fftw_arm/include
#                /usr/local/fftw_arm/lib

LIBS += -L$$PWD/deploy/ -llibfftw3-3
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS  += -fopenmp
# QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_CXXFLAGS += -Os
QMAKE_CXXFLAGS += -fdata-sections -ffunction-sections
QMAKE_LFLAGS += -Wl,--gc-sections

win32 {
    # Copy runtime resources/DLLs the app loads at runtime (see
    # AutoReferenceMap::loadResourceFile and the deploy/ redistributables)
    # next to the built executable so a plain qmake+make build runs standalone.
    CONFIG(debug, debug|release) {
        DEPLOY_DIR = $$OUT_PWD/debug
    } else {
        DEPLOY_DIR = $$OUT_PWD/release
    }
    QMAKE_POST_LINK += $$quote(mkdir -p \"$$DEPLOY_DIR/resources\" \"$$DEPLOY_DIR/offline_tiles\") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(cp -f \"$$PWD/resources/high_quality.rcc\" \"$$DEPLOY_DIR/resources/\") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(cp -f \"$$PWD/deploy/\"*.dll \"$$DEPLOY_DIR/\") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(cp -f \"$$PWD/resources/Map/offline_tiles/\"*.png \"$$DEPLOY_DIR/offline_tiles/\")
}

# unix {
#     # Linux/macOS平台
#     QMAKE_POST_LINK += $$quote(mkdir -p $$OUT_PWD/resources/ && cp -f $$PWD/resources/high_quality.rcc $$OUT_PWD/resources/)
# }
