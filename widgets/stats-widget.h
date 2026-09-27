#ifndef STATS_WIDGET_H
#define STATS_WIDGET_H

#include "light-common.h"
#include "recent-apps-widget.h"
#include "system-info-widget.h"
#include "host-widget.h"

class StatsWidget {
private:
    GtkWidget *window;
    GtkWidget *root_vbox;
    
    std::unique_ptr<RecentAppsWidget> recent_apps_widget;
    std::unique_ptr<SystemInfoWidget> system_info_widget;
    std::unique_ptr<HostWidget> host_widget;
    
    bool is_visible = false;
    
public:
    StatsWidget();
    void setup_window();
    void setup_ui();
    void apply_styles();
    void show();
    void hide();
    bool get_visible() const;
};

#endif // STATS_WIDGET_H


