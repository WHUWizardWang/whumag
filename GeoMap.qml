import QtQuick 2.0
//import QtQuick.Window 2.14
import QtLocation 5.6
import QtPositioning 5.6

Rectangle {
    visible: true
    width: 800
    height: 600

    Plugin {
        id: mapPlugin
        name: "mapboxgl" // "mapboxgl", "esri", ...
    }

    Map {
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(59.91, 10.75) // Oslo
        zoomLevel: 14
    }

}
