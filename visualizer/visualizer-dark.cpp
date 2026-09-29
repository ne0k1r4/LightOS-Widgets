#include <gtkmm.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <pulse/pulseaudio.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <thread>
#include <atomic>


#include <fstream>
#include <string>

struct RGB {
    double r, g, b;
};

struct Palette {
    RGB low, medium, high;
};

static RGB readColor(const std::string& name, RGB fallback) {
    std::ifstream file(
        std::string(g_get_user_config_dir()) +
        "/waybar/wallpaper-colors.css"
    );

    std::string line;
    const std::string key = "@define-color " + name;

    while (std::getline(file, line)) {
        if (line.find(key) == std::string::npos)
            continue;

        auto pos = line.find('#');
        if (pos == std::string::npos || pos + 7 > line.size())
            continue;

        try {
            unsigned int value =
                std::stoul(line.substr(pos + 1, 6), nullptr, 16);

            return {
                ((value >> 16) & 0xff) / 255.0,
                ((value >> 8) & 0xff) / 255.0,
                (value & 0xff) / 255.0
            };
        } catch (...) {
            return fallback;
        }
    }

    return fallback;
}

static Palette readPalette() {
    // low: deep/dark shade for short bars
    // medium: soft mid tone
    // high: pale/bright for tall bars
    RGB deep = readColor("wallpaper_accent_deep", {0.10, 0.10, 0.20});
    RGB soft = readColor("wallpaper_accent_soft", {0.55, 0.50, 0.65});
    RGB pale = readColor("wallpaper_accent_pale", {0.85, 0.82, 0.90});

    // If deep is nearly black, brighten slightly so bars are visible
    double lum_deep = 0.2126 * deep.r + 0.7152 * deep.g + 0.0722 * deep.b;
    if (lum_deep < 0.08)
        deep = { deep.r * 3.0, deep.g * 3.0, deep.b * 3.0 };

    return {deep, soft, pale};
}

class AudioMeter {
public:
    AudioMeter() : peak(0.0f), connected(false) {
        mainloop = pa_mainloop_new();
        context = pa_context_new(pa_mainloop_get_api(mainloop), "Visualizer");
        pa_context_set_state_callback(context, context_cb, this);
        pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
        
        thread = std::thread([this]() {
            while (running) {
                pa_mainloop_iterate(mainloop, 0, nullptr);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    ~AudioMeter() {
        running = false;
        if (thread.joinable()) thread.join();
        if (stream) { pa_stream_disconnect(stream); pa_stream_unref(stream); }
        if (context) { pa_context_disconnect(context); pa_context_unref(context); }
        if (mainloop) pa_mainloop_free(mainloop);
    }

    float get_peak() { return peak; }
    bool has_audio() { return connected && peak > 0.001f; }

private:
    pa_mainloop* mainloop = nullptr;
    pa_context* context = nullptr;
    pa_stream* stream = nullptr;
    std::atomic<float> peak;
    std::atomic<bool> connected;
    std::atomic<bool> running{true};
    std::thread thread;

    static void context_cb(pa_context* c, void* data) {
        auto* self = static_cast<AudioMeter*>(data);
        if (pa_context_get_state(c) == PA_CONTEXT_READY) {
            pa_context_get_source_info_list(c, source_cb, data);
        }
    }

    static void source_cb(pa_context* c, const pa_source_info* i, int eol, void* data) {
        if (eol || !i || !strstr(i->name, ".monitor")) return;
        
        auto* self = static_cast<AudioMeter*>(data);
        pa_sample_spec ss = {PA_SAMPLE_FLOAT32LE, 44100, 2};
        self->stream = pa_stream_new(c, "Stream", &ss, nullptr);
        pa_stream_set_read_callback(self->stream, read_cb, self);
        
        pa_buffer_attr attr = {(uint32_t)-1, 4096, 0, 0, 0};
        pa_stream_connect_record(self->stream, i->name, &attr, PA_STREAM_PEAK_DETECT);
        self->connected = true;
    }

    static void read_cb(pa_stream* s, size_t len, void* data) {
        auto* self = static_cast<AudioMeter*>(data);
        const void* buffer;
        size_t size;
        
        if (pa_stream_peek(s, &buffer, &size) >= 0 && buffer) {
            const float* samples = static_cast<const float*>(buffer);
            float max_val = 0.0f;
            for (size_t i = 0; i < size / sizeof(float); ++i) {
                max_val = std::max(max_val, std::abs(samples[i]));
            }
            self->peak = self->peak * 0.7f + max_val * 0.3f;
            pa_stream_drop(s);
        }
    }
};

class Visualizer : public Gtk::DrawingArea {
public:
    Visualizer(AudioMeter& m) : meter(m), heights(48, 0.0f) {
        set_size_request(-1, 200);

        // Load both images shown floating above wave peaks
        std::string cfg = std::string(g_get_user_config_dir()) + "/Light/assets/settings/";
        try { image_a = Gdk::Pixbuf::create_from_file(cfg + "elyfly.png"); }
        catch (...) { std::cerr << "Visualizer: elyfly.png not found\n"; }
        try { image_b = Gdk::Pixbuf::create_from_file(cfg + "elyhoc.png"); }
        catch (...) { std::cerr << "Visualizer: elyhoc.png not found\n"; }

        Glib::signal_timeout().connect([this]() {
            float peak = meter.get_peak();
            for (int i = 0; i < 48; ++i) {
                // Slight variation per band so the wave has natural shape
                float band_boost = 1.0f + 0.3f * std::sin(i * 0.4f);
                float target = peak * get_height() * 0.85f * band_boost;
                heights[i] = heights[i] * 0.70f + target * 0.30f;
            }
            queue_draw();
            return true;
        }, 33); // ~30 FPS for smooth wave
    }

private:
    AudioMeter& meter;
    std::vector<float> heights;
    Glib::RefPtr<Gdk::Pixbuf> image_a;  // elyfly.png
    Glib::RefPtr<Gdk::Pixbuf> image_b;  // elyhoc.png

    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override {
        int width  = get_allocation().get_width();
        int height = get_allocation().get_height();

        // Clear to transparent
        cr->set_source_rgba(0, 0, 0, 0);
        cr->paint();

        const Palette colors = readPalette();
        const int N = (int)heights.size(); // 48 bands
        const float step = (float)width / (N - 1);

        // Build wave point array: y position for each band
        std::vector<float> py(N);
        for (int i = 0; i < N; ++i)
            py[i] = height - std::max(4.0f, heights[i]);

        // --- Filled wave with vertical gradient ---
        // Construct a closed path: wave crest + bottom corners
        cr->move_to(0, height);
        cr->line_to(0, py[0]);

        // Smooth cubic bezier through all points
        for (int i = 0; i < N - 1; ++i) {
            float x0 = i * step;
            float x1 = (i + 1) * step;
            float cp_offset = step * 0.4f;
            cr->curve_to(
                x0 + cp_offset, py[i],
                x1 - cp_offset, py[i + 1],
                x1,             py[i + 1]
            );
        }

        cr->line_to(width, height);
        cr->close_path();

        // Vertical gradient fill: pale at top, deep at bottom
        auto grad = Cairo::LinearGradient::create(0, 0, 0, height);
        grad->add_color_stop_rgba(0.0, colors.high.r,   colors.high.g,   colors.high.b,   0.85);
        grad->add_color_stop_rgba(0.5, colors.medium.r, colors.medium.g, colors.medium.b, 0.60);
        grad->add_color_stop_rgba(1.0, colors.low.r,    colors.low.g,    colors.low.b,    0.30);
        cr->set_source(grad);
        cr->fill_preserve();

        // --- Glowing wave outline stroke ---
        // Redraw just the wave curve (no bottom)
        cr->move_to(0, py[0]);
        for (int i = 0; i < N - 1; ++i) {
            float x0 = i * step;
            float x1 = (i + 1) * step;
            float cp_offset = step * 0.4f;
            cr->curve_to(
                x0 + cp_offset, py[i],
                x1 - cp_offset, py[i + 1],
                x1,             py[i + 1]
            );
        }
        // Outer glow (wide, semi-transparent)
        cr->set_source_rgba(colors.high.r, colors.high.g, colors.high.b, 0.35);
        cr->set_line_width(6.0);
        cr->stroke_preserve();
        // Inner bright line
        cr->set_source_rgba(1.0, 1.0, 1.0, 0.55);
        cr->set_line_width(1.5);
        cr->stroke();

        // --- Images floating above wave peaks (every 8th band) ---
        const int img_size = 32;
        for (int i = 0; i < N; i += 8) {
            auto& img = (i % 16 == 0) ? image_a : image_b;
            if (!img) continue;
            float x = i * step;
            float y = py[i] - img_size - 6;
            if (y < 0) y = 0;
            auto scaled = img->scale_simple(img_size, img_size, Gdk::INTERP_BILINEAR);
            Gdk::Cairo::set_source_pixbuf(cr, scaled, (int)(x - img_size / 2), (int)y);
            cr->paint_with_alpha(0.85);
        }

        if (!meter.has_audio()) {
            cr->set_source_rgba(colors.medium.r, colors.medium.g, colors.medium.b, 0.7);
            cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL,
                                 Cairo::FONT_WEIGHT_NORMAL);
            cr->set_font_size(12);
            cr->move_to(width - 120, 20);
            cr->show_text("No audio");
        }

        return true;
    }
};

class App : public Gtk::Application {
public:
    App() : Gtk::Application("org.lightos.Visualizer") {}

    void on_activate() override {
        meter = std::make_unique<AudioMeter>();
        
        auto* window = new Gtk::Window();
        window->set_default_size(-1, 200);
        window->set_decorated(false);
        window->set_opacity(0.9);
        window->set_accept_focus(false);
        window->set_app_paintable(true);

        auto screen = window->get_screen();
        auto visual = screen->get_rgba_visual();
        if (visual) {
            gtk_widget_set_visual(GTK_WIDGET(window->gobj()), visual->gobj());
        }

        GtkWindow* gtk_win = GTK_WINDOW(window->gobj());
        gtk_layer_init_for_window(gtk_win);
        gtk_layer_set_layer(gtk_win, GTK_LAYER_SHELL_LAYER_BOTTOM);
        gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
        gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_LEFT, true);
        gtk_layer_set_anchor(gtk_win, GTK_LAYER_SHELL_EDGE_RIGHT, true);
        gtk_layer_set_margin(gtk_win, GTK_LAYER_SHELL_EDGE_BOTTOM, 0);

        auto* vis = new Visualizer(*meter);
        window->add(*vis);
        
        add_window(*window);
        window->show_all();
    }

private:
    std::unique_ptr<AudioMeter> meter;
};

int main(int argc, char* argv[]) {
    auto app = Glib::RefPtr<App>(new App());
    return app->run(argc, argv);
}
