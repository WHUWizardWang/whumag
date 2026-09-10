import QtQuick 2.5
import QtQuick.Controls 1.4

Menu {
    property  var coordinate
    property int drawState
    signal itemClicked(int state)
    signal magItemClicked(string item)

    function update(){
        clear();
        switch (drawState) {
        case 0:
            addItem(qsTr("绘制多边形")).triggered.connect(function(){itemClicked(0)})
            break
        case 2:
            addItem(qsTr("EMAG2")).triggered.connect(function(){magItemClicked("EMAG2")})
            addItem(qsTr("MAMEA")).triggered.connect(function(){magItemClicked("MAMEA")})
            addItem(qsTr("清除多边形")).triggered.connect(function(){itemClicked(2)})
            break;
        default:
            console.log("Unsupported MapPopupMenu State")
        }
    }
}
