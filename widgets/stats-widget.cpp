#include "stats-widget.h"

StatsWidget::StatsWidget() {
    setup_window();
    setup_ui();
    apply_styles();
    
    // Create sub-widgets
    recent_apps_widget = std::make_unique<RecentAppsWidget>();
    system_info_widget = std::make_unique<SystemInfoWidget>();
    host_widget = std::make_unique<HostWidget>();
    
    // Connect host widget to system info widget for art refresh
    host_widget->set_system_info_widget(system_info_widget.get());
    
    gtk_box_pack_start(GTK_BOX(root_vbox), recent_apps_widget->get_widget(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root_vbox), system_info_widget->get_widget(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root_vbox), host_widget->get_widget(), FALSE, FALSE, 0);
}

void StatsWidget::setup_window() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Stats Widget");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 420); // Taller for vertical layout
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);

    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE); // Keep on left side
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 20); // Original margin
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 20); // Original margin

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(window, visual);
    }

    gtk_widget_set_app_paintable(window, TRUE);
}

void StatsWidget::setup_ui() {
    root_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12); // Slightly more spacing
    gtk_container_add(GTK_CONTAINER(window), root_vbox);
}

void StatsWidget::apply_styles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    std::string css;
    if (g_use_hoc_theme) {
        css = R"(
            window { background-color: rgba(0,0,0,0); }
            #apps-header-label, #system-header-label { font-family: LightOSNew12; font-size: 13px; font-weight: 700; color: #2a2a2a; letter-spacing: 0.5px; }
            #app-button { 
                background-color: rgba(255,255,255,0.1); 
                border-radius: 28px; 
                border: none; 
                transition: all 0.3s ease; 
                padding: 8px; 
                transform: scale(1.0);
            }
            #app-button:hover { 
                background-color: rgba(255,255,255,0.25); 
                transform: scale(1.05);
            }
            #app-icon { font-size: 20px;}
            #app-icon-fallback { font-size: 20px; }
            #app-name { font-family: LightOSNew12; font-size: 9px; font-weight: 500; color: #3a3a3a; }
            #system-label { font-family: LightOSNew12; font-size: 11px; font-weight: 600; color: #2a2a2a; text-align: center; }
        )";
    } else {
        css = R"(
            window { background-color: rgba(0,0,0,0); }
            #apps-header-label, #system-header-label { font-family: LightOSNew12; font-size: 13px; font-weight: 700; color: #2a2a2a; letter-spacing: 0.5px; }
            #app-button { 
                background-color: rgba(255,255,255,0.1); 
                border-radius: 28px; 
                border: none; 
                transition: all 0.3s ease; 
                padding: 8px; 
                transform: scale(1.0);
            }
            #app-button:hover { 
                background-color: rgba(255,255,255,0.25); 
                transform: scale(1.05);
            }
            #app-icon { font-size: 20px;}
            #app-icon-fallback { font-size: 20px; }
            #app-name { font-family: LightOSNew12; font-size: 9px; font-weight: 500; color: #3a3a3a; }
            #system-label { font-family: LightOSNew12; font-size: 11px; font-weight: 600; color: #2a2a2a; text-align: center; }
        )";
    }
    
    gtk_css_provider_load_from_data(provider, css.c_str(), -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

void StatsWidget::show() {
    if (!is_visible) {
        is_visible = true;
        refresh_global_theme();
        apply_styles();
        if (recent_apps_widget) {
            recent_apps_widget->refresh();
        }
        if (system_info_widget) {
            system_info_widget->refresh();
            system_info_widget->start_art_loading();
        }
        gtk_widget_show_all(window);
    }
}

void StatsWidget::hide() {
    if (is_visible) {
        is_visible = false;
        gtk_widget_hide(window);
    }
}

bool StatsWidget::get_visible() const {
    return is_visible;
}
