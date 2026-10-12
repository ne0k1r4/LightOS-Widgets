#include <gtkmm.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <glibmm.h>
#include <array>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <regex>

namespace {
constexpr int CW = 340, CH = 165, KW = 300, KH = 321;
std::string asset_dir() {
    if (const char* e = std::getenv("LIGHTOS_CHRONO_ASSETS")) return e;
    return std::string(g_get_user_data_dir()) + "/lightos-chrono/assets";
}
std::string asset(const std::string& file) { return asset_dir() + "/" + file; }
std::string notes_file() { return std::string(g_get_user_data_dir()) + "/lightos-chrono/notes.tsv"; }
std::tm local_now() { auto t=std::time(nullptr); std::tm out{}; localtime_r(&t,&out); return out; }
std::string fmt(const std::tm& t, const char* f) { std::ostringstream o; o << std::put_time(&t,f); return o.str(); }
void set_layer(Gtk::Window& w, int right, int top) {
    auto* native=GTK_WINDOW(w.gobj());
    gtk_layer_init_for_window(native);
    gtk_layer_set_layer(native, GTK_LAYER_SHELL_LAYER_BOTTOM);
    gtk_layer_set_anchor(native, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(native, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_margin(native, GTK_LAYER_SHELL_EDGE_RIGHT, right);
    gtk_layer_set_margin(native, GTK_LAYER_SHELL_EDGE_TOP, top);
    gtk_layer_set_exclusive_zone(native, -1);
    gtk_layer_set_keyboard_mode(native, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
}
void picture(Gtk::Image& im, const std::string& name, int w, int h) {
    try { auto pb=Gdk::Pixbuf::create_from_file(asset(name));
          im.set(pb->scale_simple(w,h,Gdk::INTERP_BILINEAR)); }
    catch(const Glib::Error& e) { g_warning("Missing/invalid asset %s: %s",name.c_str(),e.what().c_str()); }
}
std::string base_css() {
    return R"CSS(
    window { background-color: transparent; }
    button { background-image: none; background-color: transparent; border: 0; box-shadow: none; padding: 0; color: #f5d9c4; font-family: 'LightOSNew12'; }
    button:hover { color: #ff708c; text-shadow: 0 0 7px #a62c49; }
    #chrono-time { font-family: 'LightOSNew12'; font-weight: bold; font-size: 20px; color: #f8ddc3; text-shadow: 0 0 9px #9e3b49; }
    #chrono-date { font-family: 'LightOSNew12'; font-size: 9px; color: #efc6a8; text-shadow: 0 0 5px #7a263b; }
    #calendar-month { font-family: 'LightOSNew12'; font-size: 19px; color: #f6d7bd; }
    .weekday { font-family: 'LightOSNew12'; font-size: 10px; color: #dc9e95; }
    .day { font-family: 'LightOSNew12'; font-size: 11px; color: #f2d1b7; border-radius: 13px; }
    .day:hover { background-color: rgba(169, 34, 64, 0.42); }
    .today { background-color: rgba(170, 33, 61, 0.65); color: #ffffff; }
    .marked { color: #ff8798; }
    .nav { font-size: 18px; color: #f6d0af; }
    .close { font-size: 14px; color: #efb1b0; }
    )CSS";
}
}

class Chrono : public Gtk::Window {
public:
    Chrono() {
        set_title("LightOS Chrono - Book of Fate"); set_name("chrono-main");
        set_decorated(false); set_resizable(false); set_default_size(CW,CH);
        set_app_paintable(true); set_layer(*this, 18, 55);
        css_provider=Gtk::CssProvider::create();
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(),css_provider,GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        reload_palette(true);
        views.set_hhomogeneous(false);
        views.set_vhomogeneous(false);
        views.add(clock_layout,"clock");
        views.add(cal_layout,"calendar");
        add(views);
        views.set_visible_child(clock_layout);
        clock_layout.set_size_request(CW,CH);
        cal_layout.set_size_request(KW,KH);
        picture(clock_art,"clock-main.png",CW,CH); clock_layout.put(clock_art,0,0);
        time_label.set_name("chrono-time"); time_label.set_size_request(132,32);
        time_label.set_halign(Gtk::ALIGN_CENTER); clock_layout.put(time_label,181,61);
        date_button.set_name("chrono-date"); date_button.set_size_request(111,24);
        date_button.set_tooltip_text("Open Calendar of Fate");
        date_button.signal_clicked().connect(sigc::mem_fun(*this,&Chrono::open_calendar));
        clock_layout.put(date_button,183,98);
        clock_layout.put(heart_art,235,138); // heart pulses over the skull
        visual_button.set_relief(Gtk::RELIEF_NONE);
        visual_button.set_tooltip_text("Toggle LightOS audio visualizer");
        visual_button.set_image(visual_art); visual_button.set_always_show_image(true);
        visual_button.signal_clicked().connect(sigc::mem_fun(*this,&Chrono::toggle_visualizer));
        clock_layout.put(visual_button,300,133);
        close_button.set_label("×"); close_button.set_name("chrono-close");
        close_button.get_style_context()->add_class("close");
        close_button.signal_clicked().connect([this]{ close(); });
        clock_layout.put(close_button,321,2);
        // The calendar is a second view in the SAME top-level window.
        // No secondary Gtk::Window, no hide(), no application shutdown.
        picture(cal_art,"calendar-frame.png",KW,KH); cal_layout.put(cal_art,0,0);
        cal_month.set_name("calendar-month"); cal_month.set_size_request(206,30);
        cal_month.set_halign(Gtk::ALIGN_CENTER); cal_layout.put(cal_month,48,33);
        cal_prev.set_label("‹"); cal_next.set_label("›"); cal_back.set_label("×");
        for(auto* b:{&cal_prev,&cal_next,&cal_back}) b->get_style_context()->add_class("nav");
        cal_layout.put(cal_prev,28,34); cal_layout.put(cal_next,261,34); cal_layout.put(cal_back,277,3);
        cal_prev.signal_clicked().connect([this]{ change_month(-1); });
        cal_next.signal_clicked().connect([this]{ change_month(1); });
        cal_back.signal_clicked().connect(sigc::mem_fun(*this,&Chrono::close_calendar));
        const char* days[]={"S","M","T","W","T","F","S"};
        for(int i=0;i<7;++i) { auto* l=Gtk::make_managed<Gtk::Label>(days[i]);
            l->get_style_context()->add_class("weekday"); l->set_size_request(28,18);
            cal_layout.put(*l,40+i*31,81); }
        for(int i=0;i<42;++i) { auto* b=Gtk::make_managed<Gtk::Button>();
            b->set_size_request(29,26); b->get_style_context()->add_class("day");
            cal_layout.put(*b,40+(i%7)*31,102+(i/7)*28);
            b->signal_clicked().connect([this,i]{ click_day(i); }); day_buttons[i]=b; }
        cal_today.set_label("Today"); cal_today.get_style_context()->add_class("weekday");
        cal_layout.put(cal_today,37,278);
        cal_today.signal_clicked().connect([this]{ auto t=local_now(); year=t.tm_year+1900;month=t.tm_mon;render_calendar(); });
        auto t=local_now(); year=t.tm_year+1900; month=t.tm_mon;
        load_notes(); render_calendar(); update_time(); update_heart();
        visualizer_on = visualizer_running(); // First click HIDES an already-running visualizer
        update_visual_icon();
        Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this,&Chrono::update_time),1);
        Glib::signal_timeout().connect(sigc::mem_fun(*this,&Chrono::update_heart),135);
        Glib::signal_timeout().connect_seconds([this]{ reload_palette(false); return true; }, 2);
        show_all_children(); views.set_visible_child(clock_layout);
    }
private:
    Gtk::Stack views;
    Gtk::Fixed clock_layout,cal_layout;
    Glib::RefPtr<Gtk::CssProvider> css_provider;
    std::string palette_cache;
    bool calendar_open=false;
    Gtk::Image clock_art,heart_art,visual_art,cal_art;
    Gtk::Label time_label,cal_month;
    Gtk::Button date_button,visual_button,close_button,cal_prev,cal_next,cal_back,cal_today;
    std::array<Gtk::Button*,42> day_buttons{};
    std::map<std::string,std::string> notes;
    int year=2026,month=9,heart_step=0,heart_pause=0;
    bool visualizer_on=false;

    static bool visualizer_running() { return std::system("pgrep -x visualizer >/dev/null 2>&1")==0; }
    bool update_time() {
        auto t=local_now(); std::string shown=fmt(t,"%I:%M %p"); if(shown.size() && shown[0]=='0') shown.erase(0,1);
        time_label.set_text(shown);
        date_button.set_label(fmt(t,"%a, %b %d, %Y"));
        return true;
    }
    bool update_heart() {
        if(!is_visible() || calendar_open) return true;
        if(heart_pause>0) { --heart_pause; return true; }
        static const int seq[]={1,2,3,4,5,4,3,2};
        int frame=seq[heart_step];
        picture(heart_art,frame==1?"heart.png":"heart"+std::to_string(frame)+".png",19,19);
        heart_step=(heart_step+1)%8;
        if(heart_step==0) heart_pause=4;
        return true;
    }
    void update_visual_icon() {
        picture(visual_art,visualizer_on?"visualizer-active.png":"visualizer-inactive.png",35,19);
    }
    void toggle_visualizer() {
        // Treat a running visualizer as ON at startup. A single click hides it.
        // SIGUSR2 hides, SIGUSR1 shows; these are the signals used by LightOS.
        if (visualizer_on) {
            std::system("killall -SIGUSR2 visualizer >/dev/null 2>&1");
            visualizer_on = false;
        } else {
            if (visualizer_running()) {
                std::system("killall -SIGUSR1 visualizer >/dev/null 2>&1");
                visualizer_on = true;
            } else {
                std::string dir=std::string(g_get_user_config_dir())+"/Light/widgets/visualizer/";
                std::string selected=dir+"light/visualizer";
                if(const char* env=std::getenv("GTK_THEME"))
                    if(std::string(env).find("HoC")!=std::string::npos) selected=dir+"dark/visualizer";
                if(access(selected.c_str(),X_OK)==0) {
                    GError* error=nullptr;
                    const gchar* argv[]={selected.c_str(),nullptr};
                    if(g_spawn_async(nullptr,const_cast<gchar**>(argv),nullptr,
                                     G_SPAWN_SEARCH_PATH,
                                     nullptr,nullptr,nullptr,&error)) {
                        visualizer_on=true;
                    } else {
                        g_warning("Cannot start visualizer: %s",error?error->message:"unknown error");
                        if(error) g_error_free(error);
                    }
                } else g_warning("Visualizer executable unavailable: %s",selected.c_str());
            }
        }
        update_visual_icon();
    }
    void open_calendar() {
        render_calendar();
        calendar_open=true;
        gtk_layer_set_keyboard_mode(GTK_WINDOW(gobj()), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
        views.set_visible_child(cal_layout);
        resize(KW,KH);
        gtk_widget_queue_resize(GTK_WIDGET(gobj()));
    }
    void close_calendar() {
        calendar_open=false;
        gtk_layer_set_keyboard_mode(GTK_WINDOW(gobj()), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
        views.set_visible_child(clock_layout);
        resize(CW,CH);
        gtk_widget_queue_resize(GTK_WIDGET(gobj()));
    }
    bool on_key_press_event(GdkEventKey* event) override {
        if (calendar_open && event->keyval==GDK_KEY_Escape) {
            close_calendar(); return true;
        }
        return Gtk::Window::on_key_press_event(event);
    }
    void reload_palette(bool force) {
        const std::string path=std::string(g_get_home_dir())+"/.config/waybar/wallpaper-colors.css";
        std::ifstream file(path);
        std::string raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        if(!force && raw==palette_cache) return;
        palette_cache=raw;
        auto pick=[&raw](const std::string& name,const std::string& fallback) {
            std::regex pattern("@define-color\\s+"+name+"\\s+(#[0-9a-fA-F]{6})\\s*;");
            std::smatch match;
            return std::regex_search(raw,match,pattern)?match[1].str():fallback;
        };
        const std::string accent=pick("wallpaper_accent_soft","#e8b49a");
        const std::string bright=pick("wallpaper_accent_pale","#f5d9c4");
        const std::string strong=pick("wallpaper_accent","#bd3d60");
        std::string dynamic="\n#chrono-time { color: "+bright+"; text-shadow: 0 0 7px "+strong+"; }\n"
            "#chrono-date, #calendar-month, .weekday, .day, .nav { color: "+accent+"; }\n"
            ".today { background-color: "+strong+"; color: #ffffff; }\n"
            ".marked { color: "+bright+"; }\n"
            "button:hover { color: "+bright+"; }\n";
        try { css_provider->load_from_data(base_css()+dynamic); }
        catch(const Glib::Error& e) { g_warning("Chrono palette error: %s",e.what().c_str()); }
    }
    void change_month(int delta) {
        month+=delta;
        if(month<0) {month=11;--year;} else if(month>11) {month=0;++year;}
        render_calendar();
    }
    std::string key(int day) const {
        std::ostringstream o; o<<year<<'-'<<std::setw(2)<<std::setfill('0')<<month+1<<'-'<<std::setw(2)<<day;
        return o.str();
    }
    void load_notes() {
        std::ifstream f(notes_file()); std::string line;
        while(std::getline(f,line)) { auto tab=line.find('\t'); if(tab!=std::string::npos) notes[line.substr(0,tab)]=line.substr(tab+1); }
    }
    void save_notes() {
        auto dir=std::string(g_get_user_data_dir())+"/lightos-chrono";
        g_mkdir_with_parents(dir.c_str(),0700);
        std::ofstream f(notes_file(),std::ios::trunc);
        for(const auto& n:notes) f<<n.first<<'\t'<<n.second<<'\n';
    }
    void render_calendar() {
        std::tm first{}; first.tm_year=year-1900; first.tm_mon=month; first.tm_mday=1; first.tm_hour=12;
        std::mktime(&first); int start=first.tm_wday;
        std::tm next=first; next.tm_mon=month+1; std::mktime(&next);
        auto last=next; last.tm_mday=0; std::mktime(&last); int count=last.tm_mday;
        cal_month.set_text(fmt(first,"%B %Y")); auto today=local_now();
        for(int i=0;i<42;++i) {
            int d=i-start+1; auto* b=day_buttons[i];
            auto ctx=b->get_style_context(); ctx->remove_class("today");ctx->remove_class("marked");
            if(d>=1 && d<=count) {
                b->set_label(std::to_string(d)); b->set_sensitive(true);
                if(year==today.tm_year+1900 && month==today.tm_mon && d==today.tm_mday) ctx->add_class("today");
                if(notes.count(key(d)) && !notes[key(d)].empty()) ctx->add_class("marked");
            } else {b->set_label("");b->set_sensitive(false);}
        }
    }
    void click_day(int index) {
        std::tm first{};first.tm_year=year-1900;first.tm_mon=month;first.tm_mday=1;first.tm_hour=12;std::mktime(&first);
        int day=index-first.tm_wday+1;
        std::tm last=first; last.tm_mon=month+1; last.tm_mday=0; std::mktime(&last);
        if(day<1||day>last.tm_mday) return;
        std::string k=key(day);
        Gtk::Dialog dialog("Fate Diary - "+k,*this,true);
        dialog.add_button("Cancel",Gtk::RESPONSE_CANCEL);
        dialog.add_button("Save",Gtk::RESPONSE_OK);
        Gtk::Entry entry; entry.set_placeholder_text("A note for this day...");
        entry.set_text(notes.count(k)?notes[k]:"");entry.set_width_chars(34);
        dialog.get_content_area()->pack_start(entry,Gtk::PACK_SHRINK,12);
        entry.show();
        if(dialog.run()==Gtk::RESPONSE_OK) {
            std::string value=entry.get_text();
            std::replace(value.begin(),value.end(),'\t',' ');
            std::replace(value.begin(),value.end(),'\n',' ');
            if(value.empty()) notes.erase(k); else notes[k]=value;
            save_notes();render_calendar();
        }
    }
};

int main(int argc,char** argv) {
    auto app=Gtk::Application::create(argc,argv,"org.lightos.ChronoFateTest");
    Chrono chrono;
    return app->run(chrono);
}
