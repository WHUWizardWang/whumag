import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtPositioning 5.12
import QtLocation 5.12

import OmgGeoObject 1.0

Rectangle {
    id: rect
    visible: true
    width: 640
    height: 480
//    anchors.fill: parent
    signal drawStateChanged(int state)

//    Plugin{
//        id: mapPlugin
////        name: "mapboxgl"
//        name: "osm"
//        PluginParameter{
//            name: "osm.mapping.offline.directory"
//            value: ":/offline_tiles/"
//        }
//    }

    // Plugin {
    //     id: mapPlugin
    //     name: "osm";
    //     PluginParameter {
    //         name: 'osm.mapping.offline.directory'
    //         value: 'qrc:/offline_tiles/'
    //     }
    // }
    // PluginParameter {
    //     name: "osm.mapping.providersrepository.disabled"
    //     value: true
    // }

    // // PluginParameter {
    // //     name: "osm.mapping.cache.directory"
    // //     value: 'D:/Qtapp/whumag/tiles/'
    // // }

    // PluginParameter {
    //     name: "osm.mapping.online.enabled"
    //     value: false
    // }



    Map{
        id: map
        anchors.fill: parent

        activeMapType: map.supportedMapTypes[1]
        zoomLevel: 1
        plugin: Plugin {
            id: mapPlugin
            name: 'osm';
            PluginParameter {
                name: 'osm.mapping.offline.directory'
                //value: ':/offline_tiles/'
                value: Qt.application.dirPath + "/offline_tiles"
            }
            PluginParameter {
                name: "osm.mapping.providersrepository.disabled"
                value: true
            }
            PluginParameter {
                name: "osm.mapping.offline.enabled"
                value: true
            }
            PluginParameter {
                name: "osm.mapping.online.enabled"
                value: false
            }
            PluginParameter {
                name: "osm.mapping.debug"
                value: true
            }

        }
        center: QtPositioning.coordinate(20.0, 120.75)

        property int undraw : 0
        property int drawing : 1
        property int drawed : 2

        property int drawState : map.undraw

        property int emag2 : 0
        property int mamea : 1

        property int dataType : emag2

//        property list<PixelItem> pixels
        property var polygonImage

        property var msX : []
        property var msY : []
        property real radio : 1.0     // image.height / image.width

        Component.onCompleted: {
            var jsonString = omgPolygon.loadChinaBorder()
//            console.log("Tiles path:", tilesPath)

            // 检查文件是否存在
//            console.log("Sample tile exists:", Qt.resolvedUrl(tilesPath + "osm_100-1-3-0-0-0.png"))

//            console.log(jsonString)
            var jsonBorder = JSON.parse(jsonString)
//            console.log(jsonBorder.coordinates[2][0][0])
            var polygon_count = jsonBorder.coordinates.length
            for(var i=0;i<polygon_count;i++){
                var polygon = Qt.createQmlObject('import QtLocation 5.12; MapPolygon {}', map)
                polygon.color = "purple"; // 设置多边形填充颜色
                polygon.border.width = 10;
                polygon.border.color = "purple";

                // 动态生成多边形的路径，这里只是一个示例，你需要根据实际情况设置坐标
                var path = [];
                var pnt_cnt = jsonBorder.coordinates[i][0].length
                if(pnt_cnt>50 || pnt_cnt<23){   // Filter
                    continue
                }

//                console.log(pnt_cnt)
                for (var j = 0; j < pnt_cnt; ++j) {
                    var lat = jsonBorder.coordinates[i][0][j][1]
                    var lng = jsonBorder.coordinates[i][0][j][0]
                    path.push(QtPositioning.coordinate(lat, lng))
                }
//                console.log("1")
                polygon.path = path
                map.addMapItem(polygon)
            }
        }

        function showPixel(flag){
            map.calculateImageRadio()
            omgPolygon.setRadio(map.radio)
            omgPolygon.getInertnalPoints(flag)
//            mapPolygon.clear()
            map.removeMapItem(map.polygonImage)
            var raster = Qt.createQmlObject('OmgMapImage {}', map)
            raster.coordinate = QtPositioning.coordinate(omgPolygon.maxLat, omgPolygon.minLon)
            raster.setSource(omgPolygon.getResImgPath())
            if(map.dataType === map.emag2) {
                raster.zoomLevel = 5.37
            } else if(map.dataType === map.mamea) {
                raster.zoomLevel = 5.40
            }

            map.addMapItem(raster)
            map.polygonImage = raster;
        }

        function pushMousePosition(x, y){
            msX.push(x)
            msY.push(y)
        }

        function calculateImageRadio(){
            var minX = 9999.9999
            var maxX = -9999.9999
            var minY = 9999.9999
            var maxY = -9999.9999
            for(var i=0; i<msX.length; i++) {
                minX = Math.min(minX, msX[i])
                maxX = Math.max(maxX, msX[i])
                minY = Math.min(minY, msY[i])
                maxY = Math.max(maxY, msY[i])
            }
            if (minX >= maxX || minY >= maxY) {
                radio = 1
            } else {
                radio = (maxY-minY)/(maxX-minX)
            }
        }

        function checkTileFile(basePath) {
                console.log("=== 检查瓦片文件 ===")

                // 基于您的缩放级别和中心点，计算应该加载的瓦片
                var zoom = 1
                var lat = 20.0
                var lon = 120.75

                // 简单的瓦片坐标计算（这是近似的）
                var x = Math.floor((lon + 180) / 360 * Math.pow(2, zoom))
                var y = Math.floor((1 - Math.log(Math.tan(lat * Math.PI / 180) + 1 / Math.cos(lat * Math.PI / 180)) / Math.PI) / 2 * Math.pow(2, zoom))

                console.log("For zoom=" + zoom + ", lat=" + lat + ", lon=" + lon)
                console.log("Expected tile coordinates: x=" + x + ", y=" + y)

                // 检查对应的瓦片文件是否存在
                var expectedTileName = "osm_" + zoom + "-" + x + "-" + y + "-0.png"
                var tileUrl = Qt.resolvedUrl(basePath + expectedTileName)
                console.log("Looking for tile:", expectedTileName)
                console.log("Full URL:", tileUrl)
            }

        // 监听地图状态
        onMapReadyChanged: {
            console.log("Map ready:", mapReady)
        }

        onErrorChanged: {
            if (error !== Map.NoError) {
                console.error("❌ Map error:", error)
            }
        }

        function clearMousePositions() {
            msX = []
            msY = []
        }

        MouseArea{
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            property var lastCoordinate


            onClicked: {
                var pnt;
                if(mouse.button === Qt.RightButton){
                    switch (map.drawState) {
                    case map.undraw:
                        pnt = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                        mapPopupMenu.show(pnt)
                        break
                    case map.drawing:
                        mapPolygon.done()
                        mouseArea.cursorShape = Qt.ArrowCursor
                        map.drawState = map.drawed
                        drawStateChanged(map.drawed)
                        // Polygon is invalid if the number of points is less than 3.
                        if(mapPolygon.path.length < 3){
                            mapPolygon.clear()
                            map.drawState = map.undraw
                            drawStateChanged(map.undraw)
                        }

                        break
                    case map.drawed:
                        pnt = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                        mapPopupMenu.show(pnt)
                        break
                    default:
                        console.log("Unsupported DrawState")
                    }
                } else if(mouse.button === Qt.LeftButton){
                    switch (map.drawState) {
                    case map.drawing:
                        map.pushMousePosition(mouse.x, mouse.y)
                        pnt = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                        mapPolygon.addCoordinate(pnt);
                        break
                    default:
//                        console.log("Unsupported Map DrawState")
                    }
                }


            }

            onPositionChanged: {

            }
        }


//        OmgMapImage {
//            id: marker
//        }

        MapPopupMenu {
            id: mapPopupMenu

            function show(coordinate)
            {
                mapPopupMenu.coordinate=coordinate
                mapPopupMenu.drawState=map.drawState
                mapPopupMenu.update()
                mapPopupMenu.popup()
            }

            onItemClicked: {
                switch (state) {
                case 0:
//                    console.log("hi-------")
                    mapPolygon.color = Qt.rgba(1, 0.7, 0, 0.3)
                    mapPolygon.border.color = Qt.rgba(1, 1, 0, 1)
//                    mapPolygon.border.width= 10

                    map.drawState = map.drawing
                    rect.drawStateChanged(map.drawing)
                    mouseArea.cursorShape = Qt.CrossCursor
                    break
                case 2:
                    map.clearMousePositions()
                    mapPolygon.clear()
                    map.removeMapItem(map.polygonImage)
                    map.drawState = map.undraw
                    rect.drawStateChanged(map.undraw)
                    break
                default:
                    console.log("Unsupported MapPopupMenu State")
                }
            }

            onMagItemClicked: {

                console.log("=== 磁场数据处理开始 ===")
                    console.log("选择的数据类型:", item)
                    console.log("多边形顶点数:", mapPolygon.path.length)

                    // 打印所有顶点坐标
                    for (var i = 0; i < mapPolygon.path.length; i++) {
                        var pnt = mapPolygon.path[i]
                        console.log("顶点", i, ": 纬度=", pnt.latitude, ", 经度=", pnt.longitude)
                    }

                omgPolygon.clearNodes()
                var i;
                for(i=0; i<mapPolygon.path.length; i++){
                    var pnt = mapPolygon.path[i]
                    omgPolygon.addNode(pnt.latitude, pnt.longitude)
                }
                if (mapPolygon.path.length > 0){
                    pnt = mapPolygon.path[0]
                    omgPolygon.addNode(pnt.latitude, pnt.longitude)
                }

                switch (item) {
                case "EMAG2":
                    map.dataType = map.emag2
                    map.showPixel(0)
                    break
                case "MAMEA":
                    map.dataType = map.mamea
                    map.showPixel(1)
                    break
                default:
                    console.log("Unsupported MapPopupMenu State")
                }
                // 显示图像后多边形设置为透明
                mapPolygon.color = Qt.rgba(0, 0, 0, 0)
                mapPolygon.border.color = Qt.rgba(0, 0, 0, 0)
            }
        }


        MapPolygon {
            id: mapPolygon
            color: Qt.rgba(1, 0.7, 0, 0.3)
            border.color: Qt.rgba(1, 0, 0, 1)
            border.width: 2
            z: 1

            function clear(){
                for(var i=mapPolygon.path.length-1; mapPolygon.path.length>0; i--){
                    mapPolygon.removeCoordinate(mapPolygon.path[i]);
                }
                color = Qt.rgba(1, 0.7, 0, 0)
                border.color = Qt.rgba(1, 0, 0, 1)
            }

            function done(){
                color = Qt.rgba(0, 0.7, 1, 0.3)
                border.color = Qt.rgba(0, 0, 1, 1)
            }

        }

        OmgQmlPolygon {
            id: omgPolygon

        }
        property var lineIdArr: []


        Connections{
            target: qmlPolygon
            onSigAddVesselPath: {

                // Parse the Json string of VesselPath object
                var vessel_path = JSON.parse(json_str)
                var polyline = Qt.createQmlObject('import QtLocation 5.12; MapPolyline {}', map)
                polyline.line.width = vessel_path.width
                polyline.line.color = vessel_path.color

                var path = [];
                for(var i=0;i<vessel_path.coordinates.length;++i){
                    var lat = vessel_path.coordinates[i][0]
                    var lon = vessel_path.coordinates[i][1]
                    path.push(QtPositioning.coordinate(lat, lon))
                }
                polyline.path=path
                map.addMapItem(polyline)
                var line_id = {name: vessel_path.name,path:polyline}
//                lineIdArr.push(line_id)
                console.log(line_id)
                console.log(polyline.path.length)
            }
            function onSigRemoveVesselPath(name) {

            }
        }

//        Connections{
//            target: qmlPolygon
//            onSigAddVesselPath:onSigAddVesselPath2(json_str)
//        }


    }


}
