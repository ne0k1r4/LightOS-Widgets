#include <gtkmm.h>
#include <glibmm.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

class ClockWindow : public Gtk::Window {
public:
    ClockWindow()
    {
        set_title("Clock Widget");
        set_default_size(170, 400);
        set_decorated(false);
        set_resizable(false);
        set_name("clock-window");

        // Init gtk-layer-shell
        gtk_win = GTK_WINDOW(this->gobj());
        gtk_layer_init_for_window(gtk_win);
        gtk_layer_set_layer(gtk_win, GTK_LAYER_SHELL_LAYER_BACKGROUND);

        // Initialize screen tracking
        auto screen = Gdk::Screen::get_default();
        current_screen_width = screen->get_width();
        current_screen_height = screen->get_height();

        // Connect to screen size-changed signal
        screen->signal_size_changed().connect(sigc::mem_fun(*this, &ClockWindow::on_screen_size_changed));

        setup_initial_position();
        setup_ui();
    }

private:
    GtkWindow* gtk_win;
    Gtk::Fixed* layout;
    Gtk::Image* bg_image;
    Gtk::Label clock_label;
    Gtk::Label date_label;
    Gtk::Button close_button;
    Gtk::Button hide_button;
    Gtk::Button visualizer_button;
    bool buttons_visible = false;
    bool visualizer_hidden = false;
    int last_hour = -1;
    
    // Screen tracking variables
    int current_screen_width;
    int current_screen_height;
    
    // Widget dimensions (constants)
    static constexpr int widget_width = 170;
    static constexpr int widget_height = 400;

    void setup_initial_position() {
        update_position();
        gtk_layer_set_exclusive_zone(gtk_win, -1);
    }

    void update_position() {
        auto screen = Gdk::Screen::get_default();
        int screen_width = screen->get_width();
        int screen_height = screen->get_height();
        
        // Calculate position based on screen resolution
        int x_position, y_position;
        
        if (screen_width >= 1920 && screen_height >= 1080) {
            // Large screens: use percentage-based positioning
            double x_percentage = 0.930;  // 1785/1920 = 0.929
            double y_percentage = 0.049;  // 55/1080 = 0.051
            x_position = static_cast<int>(screen_width * x_percentage);
            y_position = static_cast<int>(screen_height * y_percentage);
        } else {
            // Smaller screens: position in top-right corner with safe margins
            x_position = screen_width - widget_width - 20;  // 20px margin from right edge
            y_position = 20;  // 20px margin from top
        }
        
        // Final boundary checks to ensure widget stays on screen
        x_position = std::max(10, x_position);
        y_position = std::max(10, std::min(y_position, screen_height - widget_height - 10));

        // Debug output
        g_print("Screen: %dx%d, Widget repositioned to: %d,%d\n", 
                screen_width, screen_height, x_position, y_position);

        // Update layer shell positioning
        gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_TOP, true);
        gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_RIGHT, true);
        gtk_layer_set_margin(gtk_win, GTK_LAYER_SHELL_EDGE_RIGHT, screen_width - x_position - widget_width);
        gtk_layer_set_margin(gtk_win, GTK_LAYER_SHELL_EDGE_TOP, y_position);
    }

    void on_screen_size_changed() {
        auto screen = Gdk::Screen::get_default();
        int new_width = screen->get_width();
        int new_height = screen->get_height();
        
        // Check if resolution actually changed
        if (new_width != current_screen_width || new_height != current_screen_height) {
            g_print("Resolution changed from %dx%d to %dx%d\n", 
                    current_screen_width, current_screen_height, new_width, new_height);
            
            current_screen_width = new_width;
            current_screen_height = new_height;
            
            // Update position with new resolution
            update_position();
        }
    }

    void setup_ui() {
        apply_css();

        // Layout container
        layout = Gtk::make_managed<Gtk::Fixed>();
        add(*layout);

        // Background image (0,0 relative to window)
        bg_image = nullptr;  // Initialize to null first
        update_background_image();
        layout->put(*bg_image, 0, 0);  // Put inside window

        // Clock labels (positioned relative to image)
        clock_label.set_name("clock-label");
        date_label.set_name("date-label");

        layout->put(clock_label, 30, 115);  // These coords are INSIDE the 170x400 window
        layout->put(date_label, 25, 150);

        // Buttons
        close_button.set_label("✕");
        close_button.set_name("close-button");
        close_button.signal_clicked().connect([] {
            std::system("pkill -f clock_widget");
        });

        hide_button.set_label("-");
        hide_button.set_name("hide-button");
        hide_button.signal_clicked().connect(sigc::mem_fun(*this, &ClockWindow::hide_buttons));

        // Visualizer toggle button
        visualizer_button.set_label("♪");
        visualizer_button.set_name("visualizer-button");
        visualizer_button.signal_clicked().connect(sigc::mem_fun(*this, &ClockWindow::toggle_visualizer));

        layout->put(close_button, 10, 10);
        layout->put(hide_button, 10, 45);
        layout->put(visualizer_button, 10, 80);  // Position below hide button

        close_button.set_visible(false);
        hide_button.set_visible(false);
        visualizer_button.set_visible(false);

        add_events(Gdk::BUTTON_PRESS_MASK);
        signal_button_press_event().connect(sigc::mem_fun(*this, &ClockWindow::on_click));

        update_time();
        Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &ClockWindow::update_time), 1);

        show_all_children();
        close_button.hide();
        hide_button.hide();
        visualizer_button.hide();
    }

    void update_background_image() {
        auto now = std::time(nullptr);
        auto* t = std::localtime(&now);
        int current_hour = t->tm_hour;
        
        // Calculate which clock image to use (1-13 based on hour)
        int clock_number = (current_hour % 13) + 1;
        
        std::string config_dir = g_get_user_config_dir();
        std::string clock_path = config_dir + "/Light/assets/clocks/clock" + std::to_string(clock_number) + ".png";
        
        try {
            auto pixbuf = Gdk::Pixbuf::create_from_file(clock_path)->scale_simple(140, 300, Gdk::INTERP_BILINEAR);
            
            if (bg_image == nullptr) {
                bg_image = Gtk::make_managed<Gtk::Image>(pixbuf);
            } else {
                bg_image->set(pixbuf);
            }
        } catch (const Glib::Error& e) {
            // Fallback to original image if clock image doesn't exist
            std::string fallback_path = config_dir + "/Light/assets/clocks/clock1.png";
            auto pixbuf = Gdk::Pixbuf::create_from_file(fallback_path)->scale_simple(140, 300, Gdk::INTERP_BILINEAR);
            
            if (bg_image == nullptr) {
                bg_image = Gtk::make_managed<Gtk::Image>(pixbuf);
            } else {
                bg_image->set(pixbuf);
            }
        }
    }

    void toggle_visualizer() {
        if (visualizer_hidden) {
            // Get current GTK theme
            std::string theme;
            FILE* pipe = popen("gsettings get org.gnome.desktop.interface gtk-theme", "r");
            if (pipe) {
                char buffer[128];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    theme = std::string(buffer);
                    theme.erase(std::remove(theme.begin(), theme.end(), '\''), theme.end());
                    theme.erase(std::remove(theme.begin(), theme.end(), '\n'), theme.end());
                }
                pclose(pipe);
            }

            // Determine visualizer path based on theme
            std::string config_dir = g_get_user_config_dir();
            std::string visualizer_path;
            if (theme == "LightOS-HoC") {
                visualizer_path = config_dir + "/Light/widgets/visualizer/dark/visualizer";
            } else if (theme == "LightOS") {
                visualizer_path = config_dir + "/Light/widgets/visualizer/light/visualizer";
            } else {
                visualizer_path = config_dir + "/Light/widgets/visualizer/light/visualizer";
            }

            // Show visualizer - try signal first, if no process found, start it
            int result = std::system("killall -SIGUSR1 visualizer 2>/dev/null");
            if (result != 0) {
                std::string command = visualizer_path + " &";
                std::system(command.c_str());
            }

            visualizer_button.set_label("♪");
            visualizer_hidden = false;
        } else {
            // Hide visualizer - send SIGUSR2 signal
            std::system("killall -SIGUSR2 visualizer 2>/dev/null");
            visualizer_button.set_label("♫");
            visualizer_hidden = true;
        }
        hide_buttons();
    }

    void hide_buttons() {
        close_button.set_visible(false);
        hide_button.set_visible(false);
        visualizer_button.set_visible(false);
        buttons_visible = false;
    }

    bool on_click(GdkEventButton*) {
        if (!buttons_visible) {
            close_button.set_visible(true);
            hide_button.set_visible(true);
            visualizer_button.set_visible(true);
            buttons_visible = true;
        }
        return true;
    }

    bool update_time()
    {
        auto now = std::time(nullptr);
        auto* t = std::localtime(&now);

        // Check if hour has changed and update background image
        if (last_hour != t->tm_hour) {
            update_background_image();
            last_hour = t->tm_hour;
        }

        std::ostringstream time_ss;
        time_ss << std::put_time(t, "%I:%M.%p");

        std::ostringstream date_ss;
        date_ss << std::put_time(t, "%A, %b %d");

        clock_label.set_text(time_ss.str());
        date_label.set_text(date_ss.str());

        return true;
    }

    void apply_css()
    {
        auto css = Gtk::CssProvider::create();
        css->load_from_data(R"(
            window#clock-window {
                background-color: transparent;
                border-radius: 16px;
            }

            #clock-label {
                color: white;
                font-family: LightOSNew12;
                font-size: 19px;
                font-weight: bold;
                text-shadow: 0 0 6px rgba(255, 255, 255, 0.7);
            }

            #date-label {
                color: white;
                font-family: LightOSNew12;
                font-size: 11px;
                font-weight: bold;
                text-shadow: 0 0 6px rgba(255, 255, 255, 0.6);
            }

            #close-button, #hide-button, #visualizer-button {
                background-color: white;
                color: black;
                border-radius: 999px;
                font-weight: bold;
                min-width: 24px;
                min-height: 24px;
                padding: 0;
            }

            #close-button:hover {
                background-color: #ff5555;
                color: white;
            }

            #hide-button:hover {
                background-color: #999999;
                color: white;
            }

            #visualizer-button:hover {
                background-color: #FD84CB;
                color: white;
            }
        )");
        Gtk::StyleContext::add_provider_for_screen(
            Gdk::Screen::get_default(),
            css,
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
};

class ClockApp : public Gtk::Application {
protected:
    ClockApp() : Gtk::Application("org.light.ClockWidget") {}

    void on_activate() override {
        auto* window = new ClockWindow();
        add_window(*window);
        window->show();
    }

public:
    static Glib::RefPtr<ClockApp> create() {
        return Glib::RefPtr<ClockApp>(new ClockApp());
    }
};

int main(int argc, char* argv[]) {
    auto app = ClockApp::create();
    return app->run(argc, argv);
}
