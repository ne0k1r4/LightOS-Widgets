import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Io

ShellRoot {
 id: root
 property bool hovered: false
 property bool pinned: false
 property bool expanded: hovered || pinned || popover !== "" || notificationActive
 property real revealProgress: expanded ? 1.0 : 0.0
 property real shimmerX: -120
 property bool shimmerRunning: false
 property string popover: ""
 property string clock: Qt.formatTime(new Date(), "hh:mm AP")
 property string dateText: Qt.formatDate(new Date(), "dddd, MMMM d")
 property string battery: "--"
 property bool charging: false
 property string title: "Nothing playing"
 property string artist: ""
 property string cover: ""
 property string wifi: ""
 property string volume: ""
 property string toast: ""
 property string notificationCount: "0"
 property string notificationPreview: ""
 property var notificationItems: []
 property int calendarOffset: 0
 property bool mediaPlaying: false
 property real audioVolume: 50
 property bool popupAnimating: false
 property int previousNotificationCount: -1
 property bool notificationActive: false
 property bool previousExpanded: false
 property real notificationReveal: notificationActive ? 1 : 0
 property string notificationText: ""
 property bool ornamentGlow: false
 property bool mediaHasCover: cover !== "" && title !== "Nothing playing"
 property var wallpaperList: []
 property int selectedWallpaper: 0
 property var wifiNetworks: []
 property var bluetoothDevices: []
 property bool wifiEnabled: false
 property bool bluetoothEnabled: false
 property string wifiSearch: ""
 property string bluetoothSearch: ""
 property string wifiPassword: ""
 property string networkFeedback: ""
 property bool networkBusy: false
 property string networkScript: Qt.resolvedUrl("scripts/network.py").toString().replace("file://", "")
 function networkAction(args) {
   if (networkBusy) return
   networkBusy = true
   networkCommand.command = ["python3", networkScript].concat(args)
   networkCommand.running = true
 }
 function refreshNetworks() {
   if (popover === "wifi" && !wifiNetworkQuery.running) wifiNetworkQuery.running = true
   if (popover === "bluetooth" && !bluetoothNetworkQuery.running) bluetoothNetworkQuery.running = true
 }
 property string home: Quickshell.env("HOME")
 property string script: Qt.resolvedUrl("scripts/control.py").toString().replace("file://", "")
 function action(args) { task.command = ["python3", script].concat(args); task.running = true }
 function open(name) { popover = popover === name ? "" : name; networkFeedback=""; refreshNetworks() }
 Behavior on revealProgress { NumberAnimation { duration: root.expanded ? 550 : 350; easing.type: root.expanded ? Easing.OutCubic : Easing.InCubic } }
 onExpandedChanged: { if (expanded) { shimmerX = -120; shimmerRunning = true; shimmer.restart() } else { shimmerRunning = false; shimmer.stop() } }
 NumberAnimation { id: shimmer; target: root; property: "shimmerX"; from: -120; to: 820; duration: 720; easing.type: Easing.OutCubic; onFinished: root.shimmerRunning = false }
 Process { id: task }
 Process {
   id: status
   command: ["python3", root.script, "status"]
   stdout: StdioCollector { onStreamFinished: {
      try { let d=JSON.parse(this.text); root.battery=d.battery; root.charging=d.charging; root.title=d.title; root.artist=d.artist; root.cover=d.cover; root.wifi=d.wifi; root.volume=d.volume; root.audioVolume=(d.volumePercent === undefined ? 50 : d.volumePercent); root.mediaPlaying=d.playing || false; root.notificationCount=String(d.notificationCount || 0); root.notificationItems=d.notifications || []; if (root.previousNotificationCount >= 0 && Number(root.notificationCount)>root.previousNotificationCount && !root.pinned && root.popover === "") { root.notificationText=d.notificationPreview || "New notification"; root.notificationActive=true; notificationTimer.restart() } root.previousNotificationCount=Number(root.notificationCount); if (d.notificationPreview && d.notificationPreview !== root.notificationPreview) { root.notificationPreview=d.notificationPreview; if (!root.pinned && root.popover === "" && !root.notificationActive) { root.toast=d.notificationPreview; toastTimer.restart() } } } catch(e) {}
   }}
 }
 Process {
   id: wifiNetworkQuery
   command: ["python3", root.networkScript, "wifi-list"]
   stdout: StdioCollector { onStreamFinished: {
     try { let d=JSON.parse(this.text); root.wifiEnabled=d.enabled; root.wifiNetworks=d.items || [] } catch(e) { root.networkFeedback="Wi-Fi list unavailable" }
   }}
 }
 Process {
   id: bluetoothNetworkQuery
   command: ["python3", root.networkScript, "bt-list"]
   stdout: StdioCollector { onStreamFinished: {
     try { let d=JSON.parse(this.text); root.bluetoothEnabled=d.enabled; root.bluetoothDevices=d.items || [] } catch(e) { root.networkFeedback="Bluetooth list unavailable" }
   }}
 }
 Process {
   id: networkCommand
   stdout: StdioCollector { onStreamFinished: {
     try { let d=JSON.parse(this.text); root.networkFeedback=d.message || (d.ok ? "Done" : "Action failed") } catch(e) { root.networkFeedback="Action completed" }
     root.networkBusy=false
     root.refreshNetworks()
   }}
 }
 Timer { interval: 6000; running: root.popover === "wifi" || root.popover === "bluetooth"; repeat: true; triggeredOnStart: true; onTriggered: root.refreshNetworks() }
 Process {
   id: wallList
   command: ["python3", root.script, "wallpapers"]
   stdout: StdioCollector { onStreamFinished: {try { root.wallpaperList=JSON.parse(this.text) } catch(e) {} }}
 }
 Timer { interval: 2500; running: true; repeat: true; triggeredOnStart: true; onTriggered: status.running=true }
 Timer { interval: 1000; running: true; repeat: true; onTriggered: {root.clock=Qt.formatTime(new Date(),"hh:mm AP");root.dateText=Qt.formatDate(new Date(),"dddd, MMMM d")}}
 Timer { id: toastTimer; interval: 2600; onTriggered: root.toast="" }
 Timer { id: notificationTimer; interval: 3600; onTriggered: root.notificationActive=false }
 PanelWindow {
   id: bar
   anchors.top: true
   implicitWidth: 760
   implicitHeight: 72
   color: "transparent"
   exclusionMode: ExclusionMode.Ignore
   Rectangle {
     id: capsule
     anchors.horizontalCenter: parent.horizontalCenter
     y: 12
     width: root.expanded ? 720 : 320
     height: 44; radius: 22
     clip: true
     color: "#080405"; border.color: "#B52B3D"; border.width: 1.5
     Behavior on width { NumberAnimation { duration: root.expanded ? 550 : 350; easing.type: root.expanded ? Easing.OutCubic : Easing.InCubic } }
     Rectangle {
       anchors.fill: parent; anchors.margins: 1.5; radius: 20.5
       gradient: Gradient {
         orientation: Gradient.Horizontal
         GradientStop { position: 0.0; color: "#250507" }
         GradientStop { position: 0.22; color: "#180609" }
         GradientStop { position: 0.53; color: "#080405" }
         GradientStop { position: 0.82; color: "#1C0508" }
         GradientStop { position: 1.0; color: "#250507" }
       }
     }
     Rectangle { anchors.fill: parent; radius: 22; color: "transparent"; border.width: 1; border.color: root.expanded ? "#E17A88" : "#7A1928"; opacity: root.expanded ? .8 : .35; Behavior on opacity { NumberAnimation { duration: 450 } } }
     // One-shot antique-silver glint; dormant while idle.
     Rectangle { visible: root.shimmerRunning; x: root.shimmerX; y: 1; width: 74; height: 1.5; radius: 1; opacity: 0.72
       gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#00D8CDD0" }
            GradientStop { position: 0.5; color: "#E8DDE0" }
            GradientStop { position: 1.0; color: "#00D8CDD0" }
        }
     }
     Image { x: 5; y: 2; width: 82; height: 40; source: "assets/art/misa_v9.png"; fillMode: Image.PreserveAspectFit; opacity: 1.0; smooth: true }
     Image { anchors.right: parent.right; anchors.rightMargin: 5; y: 2; width: 58; height: 40; source: "assets/art/cross_roses_v9.png"; fillMode: Image.PreserveAspectFit; opacity: 1.0; smooth: true }
     // V9: real ornament layers above the dark glass, below interactive controls.
     Image { x: 82; y: 0; width: parent.width-145; height: 7; source: "assets/ornaments/ornate_rail_top.png"; fillMode: Image.Stretch; opacity: .8; smooth: true }
     Image { x: 82; y: 37; width: parent.width-145; height: 7; source: "assets/ornaments/ornate_rail_bottom.png"; fillMode: Image.Stretch; opacity: .8; smooth: true }
     Rectangle { anchors.fill: parent; anchors.margins: 2; radius: 20; color: "transparent"; border.color: "#E5B6B5"; border.width: 1; opacity: .43 }
     // No straight bottom line: preserve the round capsule silhouette.
     // Engraved antique-silver interior rails and central crimson seal.
     Rectangle { x: 86; y: 5; width: parent.width-150; height: 1; color: "#BFA2A2"; opacity: .45 }
     Rectangle { x: 86; y: 38; width: parent.width-150; height: 1; color: "#BFA2A2"; opacity: .45 }
     Text { anchors.horizontalCenter: parent.horizontalCenter; y: 0; text: "✦"; color: "#B52B3D"; font.pixelSize: 9; opacity: .65 }
     MouseArea { id: capsuleHover; anchors.fill: parent; hoverEnabled: true; onEntered: root.hovered=true; onExited: root.hovered=false; onClicked: root.pinned=!root.pinned }
     MouseArea { x:0; width:78; height:44; onClicked: { root.toast="Misa-Misa ♡"; toastTimer.restart() } }
     MouseArea { anchors.right: parent.right; width:58; height:44; acceptedButtons: Qt.LeftButton | Qt.RightButton; onClicked: (m) => { if(m.button===Qt.RightButton) {root.popover="gallery";wallList.running=true} else root.action(["next-wallpaper"]) } }
     Item {
       x: 86; width: parent.width-150; height: parent.height
       Column {
         visible: !root.expanded; anchors.centerIn: parent; spacing: 0
         Text { anchors.horizontalCenter: parent.horizontalCenter; text: root.clock; color: "#F8E7E8"; font.pixelSize: 14; font.bold: true }
         Text { anchors.horizontalCenter: parent.horizontalCenter; text: "✧ " + (root.charging ? "ϟ " : "") + root.battery + "% ✧"; color: Number(root.battery)<20 ? "#F06C76" : "#DFA4AB"; font.pixelSize: 9 }
       }
       Row {
         id: expandedRow
         visible: root.expanded && !root.notificationActive
         anchors.centerIn: parent
         spacing: 7
         opacity: root.revealProgress
         x: (parent.width - width) / 2 + (1 - root.revealProgress) * 14
         Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
         Repeater {
           model: ["wifi","bluetooth","volume","notification","calendar"]
           delegate: Item {
             required property string modelData
             width: 20; height: 26
             anchors.verticalCenter: parent.verticalCenter
             Image { anchors.centerIn: parent; width: 17; height: 17; source: "assets/icons/"+modelData+".png"; fillMode: Image.PreserveAspectFit; smooth: true }
             MouseArea { anchors.fill: parent; onClicked: root.open(parent.modelData) }
           }
         }
         Rectangle { width: 1; height: 20; color: "#71303A"; anchors.verticalCenter: parent.verticalCenter }
         Text { text: root.clock; color: "#F8E7E8"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter; font.family: "Sans Serif" }
         Rectangle { width: 1; height: 20; color: "#71303A"; anchors.verticalCenter: parent.verticalCenter }
         Rectangle {
           width: 27; height: 27; radius: 6; color: "#270B12"
           anchors.verticalCenter: parent.verticalCenter; clip: true
           Image {
             anchors.fill: parent; source: root.mediaHasCover ? (root.cover.startsWith("/") ? "file://"+root.cover : root.cover) : "assets/icons/play.png"
             fillMode: root.mediaHasCover ? Image.PreserveAspectCrop : Image.PreserveAspectFit
             asynchronous: true; smooth: true
           }
         }
         Column {
           width: 100; anchors.verticalCenter: parent.verticalCenter; spacing: 2
           Text { width: 100; text:root.title==="Nothing playing"?root.dateText:root.title; color:"#F8E7E8";font.pixelSize:11;elide:Text.ElideRight;maximumLineCount:1 }
           Text { width: 100; text:root.title==="Nothing playing"?"Death Note Gothic":root.artist; color:"#C59CA1";font.pixelSize:10;elide:Text.ElideRight;maximumLineCount:1 }
         }
         Rectangle { width: 1; height: 20; color: "#71303A"; anchors.verticalCenter: parent.verticalCenter }
         Repeater {
           model: ["previous","play-pause","next"]
           delegate: Item {
             required property string modelData
             width: 20; height: 26; anchors.verticalCenter: parent.verticalCenter
             Image {
               anchors.centerIn: parent; width: 17; height: 17; smooth: true
               fillMode: Image.PreserveAspectFit
               source: "assets/icons/" + (modelData === "play-pause" ? (root.mediaPlaying ? "pause" : "play") : modelData) + ".png"
             }
             MouseArea { anchors.fill: parent; onClicked: root.action(["media",parent.modelData]) }
           }
         }
         Rectangle { width: 1; height: 20; color: "#71303A"; anchors.verticalCenter: parent.verticalCenter }
         Text { text:(root.charging?"ϟ ":"")+root.battery+"%";color:Number(root.battery)<20?"#F06C76":"#F8E7E8";font.pixelSize:10;anchors.verticalCenter:parent.verticalCenter }
       }
       Item {
         visible: root.notificationActive && root.expanded
         anchors.fill: parent
         opacity: root.notificationReveal * root.revealProgress
         Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
         Row {
           anchors.centerIn: parent; spacing: 12
           Image { width: 20; height: 20; source: "assets/icons/notification.png"; anchors.verticalCenter: parent.verticalCenter }
           Text { width: 480; text: root.notificationText; elide: Text.ElideRight; color: "#F8E7E8"; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
         }
       }
     }
   }
   Rectangle {
     visible: root.toast!==""; anchors.top:capsule.bottom; anchors.topMargin:4; anchors.horizontalCenter:parent.horizontalCenter
     width:220;height:26;radius:13;color:"#470A13";border.color:"#B52B3D"
     Text { anchors.centerIn:parent;text:root.toast;color:"#F8E7E8";font.pixelSize:11 }
   }
 }
 PanelWindow {
   id: panel
   visible: root.popover !== "" && root.popover !== "gallery"
   anchors.top: true
   implicitWidth: 340
   implicitHeight: (root.popover === "wifi" || root.popover === "bluetooth") ? 320 : root.popover === "notification" ? Math.min(410, 144 + (root.notificationItems.length > 0 ? Math.min(3, root.notificationItems.length) * 69 : 90)) : (root.popover === "calendar" ? 360 : 232)
   margins.top: 68
   color: "transparent"
   exclusionMode: ExclusionMode.Ignore
   Rectangle {
     id: panelSurface
     anchors.fill: parent; anchors.margins: 9
     radius: 19; color: "#0E0608"; border.color: "#B52B3D"; border.width: 2
     clip: true
     Rectangle { anchors.fill: parent; anchors.margins: 4; radius: 15; color: "transparent"; border.color: "#9C7778"; opacity: .6 }
     Image { source: "assets/ornaments/corner_top_left.png"; width: 74; height: 60; anchors.left: parent.left; anchors.top: parent.top; opacity: .46; fillMode: Image.PreserveAspectFit }
     Image { source: "assets/ornaments/panel_rose_corner.png"; width: 70; height: 60; anchors.right: parent.right; anchors.bottom: parent.bottom; opacity: .4; fillMode: Image.PreserveAspectFit }
     ColumnLayout {
       anchors.fill: parent; anchors.margins: 17; spacing: 9
       RowLayout { Layout.fillWidth: true
         Text { text: "✝  " + (root.popover === "notification" ? "NOTIFICATIONS" : root.popover.toUpperCase()) + "  ✝"; color: "#F8E7E8"; font.family: "serif"; font.pixelSize: 16; Layout.fillWidth: true }
         Text { text: "✕"; color: "#F8E7E8"; font.pixelSize: 17; MouseArea { anchors.fill: parent; onClicked: root.popover = "" } }
       }
       Rectangle { Layout.fillWidth: true; height: 1; color: "#7D3039" }
       ColumnLayout {
         visible: root.popover === "notification"; Layout.fillWidth: true; spacing: 8
         RowLayout { Layout.fillWidth: true
           Text { text: root.notificationCount + " unread"; color: "#E5ADB5"; font.pixelSize: 11; Layout.fillWidth: true }
           Text { text: "VIEW ALL"; color: "#E5ADB5"; font.pixelSize: 11; font.bold: true
             MouseArea { anchors.fill: parent; onClicked: root.action(["notifications-open"]) }
           }
         }
         Repeater {
           model: root.notificationItems.slice(0, 3)
           delegate: Rectangle {
             required property var modelData
             Layout.fillWidth: true; Layout.preferredHeight: 61; radius: 11
             color: "#24090E"; border.color: "#7D2834"; border.width: 1
             Column { anchors.fill: parent; anchors.margins: 9; spacing: 3
               Text { width: parent.width; text: modelData.app || "Notification"; color: "#F8E7E8"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight }
               Text { width: parent.width; text: modelData.body || modelData.summary || ""; color: "#D8ADB3"; font.pixelSize: 11; elide: Text.ElideRight }
             }
           }
         }
         Rectangle {
           visible: root.notificationItems.length === 0
           Layout.fillWidth: true; Layout.preferredHeight: 72; radius: 10
           color: "#1C090D"; border.color: "#63303A"
           Column { anchors.centerIn: parent; spacing: 6
             Text { anchors.horizontalCenter: parent.horizontalCenter; text: "✧  " + (Number(root.notificationCount) ? "Notifications waiting" : "No unread notifications") + "  ✧"; color: "#F1CDD0"; font.pixelSize: 12 }
             Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Open notification center for details"; color: "#AD868C"; font.pixelSize: 10 }
           }
         }
         GothicButton { Layout.alignment: Qt.AlignHCenter; text: "✝  OPEN NOTIFICATIONS  ✝"; onClicked: root.action(["notifications-open"]) }
       }

       ColumnLayout {
         visible: root.popover === "wifi"
         Layout.fillWidth: true
         spacing: 8

         Text {
           text: root.wifiEnabled ? "✝ WI-FI ENABLED" : "✝ WI-FI DISABLED"
           color: "#E5ADB5"
         }

         RowLayout {
           GothicButton {
             text: root.wifiEnabled ? "OFF" : "ON"
             onClicked: root.networkAction(["wifi-toggle"])
           }
           GothicButton {
             text: "SCAN"
             onClicked: root.networkAction(["wifi-scan"])
           }
         }

         TextField {
           Layout.fillWidth: true

           color: "#F8E7E8"
           placeholderTextColor: "#A9858C"
           selectionColor: "#8F2938"
           selectedTextColor: "#FFFFFF"
           font.pixelSize: 12
           Layout.preferredHeight: 32
           background: Rectangle {
             radius: 9
             color: "#1A090E"
             border.color: parent.activeFocus ? "#E17A88" : "#71303A"
             border.width: 1
           }
           placeholderText: "Search Wi-Fi networks"
           onTextChanged: root.wifiSearch = text
         }

         ScrollView {
           Layout.fillWidth: true
           Layout.preferredHeight: 125
           clip: true

           Column {
             width: 280
             spacing: 5

             Repeater {
               model: root.wifiNetworks.filter(function(n) {
                 return n.name.toLowerCase().includes(root.wifiSearch.toLowerCase())
               })

               delegate: Rectangle {
                 required property var modelData
                 width: 280
                 height: 48
                 radius: 9
                 color: modelData.active ? "#381018" : "#1C090D"
                 border.color: "#85303D"

                 Row {
                   anchors.fill: parent
                   anchors.margins: 6
                   spacing: 6

                   Column {
                     width: 170
                     Text {
                       text: modelData.name
                       width: 165
                       elide: Text.ElideRight
                       color: "#F8E7E8"
                       font.pixelSize: 11
                     }
                     Text {
                       text: modelData.signal + "% · " + modelData.security
                       color: "#D5A4AD"
                       font.pixelSize: 10
                     }
                   }

                   GothicButton {
                     text: modelData.active ? "Disconnect" : "Connect"
                     onClicked: root.networkAction([
                       modelData.active ? "wifi-disconnect" : "wifi-connect",
                       modelData.name
                     ])
                   }
                 }
               }
             }
           }
         }

         TextField {
           Layout.fillWidth: true

           color: "#F8E7E8"
           placeholderTextColor: "#A9858C"
           selectionColor: "#8F2938"
           selectedTextColor: "#FFFFFF"
           font.pixelSize: 12
           Layout.preferredHeight: 32
           background: Rectangle {
             radius: 9
             color: "#1A090E"
             border.color: parent.activeFocus ? "#E17A88" : "#71303A"
             border.width: 1
           }
           placeholderText: "Wi-Fi password"
           echoMode: TextInput.Password
           onTextChanged: root.wifiPassword = text
         }

         Text {
           text: root.networkFeedback
           color: "#E5ADB5"
           font.pixelSize: 11
           Layout.fillWidth: true
         }
       }

       ColumnLayout {
         visible: root.popover === "bluetooth"
         Layout.fillWidth: true
         spacing: 8

         Text {
           text: root.bluetoothEnabled ? "✝ BLUETOOTH ENABLED" : "✝ BLUETOOTH DISABLED"
           color: "#E5ADB5"
         }

         RowLayout {
           GothicButton {
             text: root.bluetoothEnabled ? "OFF" : "ON"
             onClicked: root.networkAction(["bt-power", root.bluetoothEnabled ? "off" : "on"])
           }
           GothicButton {
             text: "SCAN"
             onClicked: root.networkAction(["bt-scan"])
           }
         }

         TextField {
           Layout.fillWidth: true

           color: "#F8E7E8"
           placeholderTextColor: "#A9858C"
           selectionColor: "#8F2938"
           selectedTextColor: "#FFFFFF"
           font.pixelSize: 12
           Layout.preferredHeight: 32
           background: Rectangle {
             radius: 9
             color: "#1A090E"
             border.color: parent.activeFocus ? "#E17A88" : "#71303A"
             border.width: 1
           }
           placeholderText: "Search Bluetooth devices"
           onTextChanged: root.bluetoothSearch = text
         }

         ScrollView {
           Layout.fillWidth: true
           Layout.preferredHeight: 125
           clip: true

           Column {
             width: 280
             spacing: 5

             Repeater {
               model: root.bluetoothDevices.filter(function(d) {
                 return d.name.toLowerCase().includes(root.bluetoothSearch.toLowerCase())
               })

               delegate: Rectangle {
                 required property var modelData
                 width: 280
                 height: 52
                 radius: 9
                 color: "#1C090D"
                 border.color: "#85303D"

                 Row {
                   anchors.fill: parent
                   anchors.margins: 6
                   spacing: 5

                   Text {
                     text: modelData.name
                     width: 145
                     elide: Text.ElideRight
                     color: "#F8E7E8"
                     font.pixelSize: 11
                   }

                   GothicButton {
                     text: modelData.connected ? "Disconnect" : "Connect"
                     onClicked: root.networkAction([
                       modelData.connected ? "bt-disconnect" : "bt-connect",
                       modelData.address
                     ])
                   }
                 }
               }
             }
           }
         }

         Text {
           text: root.networkFeedback
           color: "#E5ADB5"
           font.pixelSize: 11
         }
       }
       ColumnLayout { visible: root.popover === "volume"; Layout.fillWidth: true; spacing: 10
         Text { text: "VOLUME  ·  " + Math.round(root.audioVolume) + "%"; color: "#E5ADB5"; font.pixelSize: 12 }
         Slider { Layout.fillWidth: true; from: 0; to: 100; value: root.audioVolume; onMoved: root.action(["volume", String(Math.round(value))]) }
       }
       ColumnLayout { visible: root.popover === "calendar"; Layout.fillWidth: true; spacing: 8
         RowLayout { Layout.fillWidth: true
           Text { text: "‹"; color: "#E5ADB5"; font.pixelSize: 22; MouseArea { anchors.fill: parent; onClicked: root.calendarOffset-- } }
           Text { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: Qt.formatDate(new Date(new Date().getFullYear(), new Date().getMonth() + root.calendarOffset, 1), "MMMM yyyy"); color: "#F8E7E8"; font.pixelSize: 13 }
           Text { text: "›"; color: "#E5ADB5"; font.pixelSize: 22; MouseArea { anchors.fill: parent; onClicked: root.calendarOffset++ } }
         }
         GridLayout { columns: 7; columnSpacing: 2; rowSpacing: 3; Layout.alignment: Qt.AlignHCenter
           Repeater { model: ["S","M","T","W","T","F","S"]
             delegate: Text { required property string modelData; text: modelData; width: 34; horizontalAlignment: Text.AlignHCenter; color: "#D9A0A9"; font.pixelSize: 11 }
           }
           Repeater { model: 42
             delegate: Rectangle {
               required property int index
               property date first: new Date(new Date().getFullYear(), new Date().getMonth() + root.calendarOffset, 1)
               property int day: index - first.getDay() + 1
               property int count: new Date(first.getFullYear(), first.getMonth() + 1, 0).getDate()
               width: 34; height: 22; radius: 7
               color: day === new Date().getDate() && first.getMonth() === new Date().getMonth() && first.getFullYear() === new Date().getFullYear() ? "#8F1927" : "transparent"
               Text { anchors.centerIn: parent; text: day > 0 && day <= count ? String(day) : ""; color: "#F5DADD"; font.pixelSize: 11 }
             }
           }
         }
       }
       Item { Layout.fillHeight: true }
     }
   }
 }
 PanelWindow {
   id: gallery
   visible:root.popover==="gallery"
   anchors {top:true;bottom:true;left:true;right:true}
   color:"#C4080405"
   Rectangle {
     width:600;height:510;anchors.centerIn:parent;radius:22;color:"#100709";border.color:"#B52B3D";border.width:2
     Image { source: "assets/ornaments/corner_top_left.png"; anchors.left: parent.left; anchors.top: parent.top; width: 130; height: 80; opacity: .7 }
     Image { source: "assets/ornaments/panel_rose_corner.png"; anchors.right: parent.right; anchors.bottom: parent.bottom; width: 140; height: 100; opacity: .65 }
     Rectangle { anchors.fill: parent; anchors.margins: 4; radius: 18; color: "transparent"; border.color: "#AD7979"; opacity: .5 }
     ColumnLayout { anchors.fill:parent;anchors.margins:24;spacing:12
       RowLayout { Layout.fillWidth:true
         Text { text:"✝  MISA'S GALLERY  ✝";color:"#F8E7E8";font.pixelSize:20;Layout.fillWidth:true }
         Text { text:"✕";color:"#F8E7E8";font.pixelSize:20;MouseArea{anchors.fill:parent;onClicked:root.popover=""} }
       }
       Rectangle { Layout.fillWidth:true;height:1;color:"#71303A" }
       Image { Layout.fillWidth:true;Layout.preferredHeight:260;source:root.wallpaperList.length?"file://"+root.wallpaperList[root.selectedWallpaper]:"";fillMode:Image.PreserveAspectFit }
       Text { text:root.wallpaperList.length?root.wallpaperList[root.selectedWallpaper].split("/").pop():"Add wallpapers to ~/Pictures/Wallpapers";color:"#E3C4C7";Layout.alignment:Qt.AlignHCenter }
       Flickable {
         Layout.fillWidth: true; Layout.preferredHeight: 66
         contentWidth: thumbnails.width; contentHeight: 64; clip: true
         Row { id: thumbnails; spacing: 8
           Repeater { model: root.wallpaperList
             delegate: Rectangle {
               id: thumb
               required property string modelData
               required property int index
               width: 98; height: 60; radius: 8; color: "#470A13"
               border.width: root.selectedWallpaper === index ? 2 : 1
               border.color: root.selectedWallpaper === index ? "#F0B2B8" : "#71303A"
               Image { anchors.fill: parent; anchors.margins: 3; source: "file://" + thumb.modelData; fillMode: Image.PreserveAspectCrop; asynchronous: true }
               MouseArea { anchors.fill: parent; onClicked: root.selectedWallpaper = thumb.index }
             }
           }
         }
       }
       RowLayout { Layout.alignment:Qt.AlignHCenter;spacing:18
         GothicButton { text:"◀ Previous";onClicked:if(root.wallpaperList.length)root.selectedWallpaper=(root.selectedWallpaper+root.wallpaperList.length-1)%root.wallpaperList.length }
         GothicButton { text:"Next ▶";onClicked:if(root.wallpaperList.length)root.selectedWallpaper=(root.selectedWallpaper+1)%root.wallpaperList.length }
       }
       GothicButton { text:"✝  APPLY WALLPAPER  ✝";Layout.alignment:Qt.AlignHCenter;enabled:root.wallpaperList.length>0;onClicked:{root.action(["set-wallpaper",root.wallpaperList[root.selectedWallpaper]]);root.popover=""} }
     }
   }
 }
}
