import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

MapQuickItem {
    zoomLevel: 5.36 //emag2: 5.36; mamea: 5.40
    anchorPoint.x: 0
    anchorPoint.y: 0
    coordinate: QtPositioning.coordinate(20.0,120.75)

    sourceItem: Image {
        id: image
        cache: false
        mipmap: true
//        source: "file:///home/Qyin/QtProjects/geomag/test.png"
    }

    function setSource(filePath){
        image.source = filePath
    }

}
