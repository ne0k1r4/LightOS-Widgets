#include <gtkmm.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <pulse/pulseaudio.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <thread>
#include <atomic>

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
        
        try {
            std::string path = std::string(g_get_user_config_dir()) + "/Light/assets/settings/background-mem.png";
            image = Gdk::Pixbuf::create_from_file(path);
        } catch (...) {
            std::cerr << "Failed to load image\n";
        }
        
        Glib::signal_timeout().connect([this]() {
            float peak = meter.get_peak();
            for (int i = 0; i < 48; ++i) {
                float target = peak * (1.0f - i * 0.02f) * get_height() * 1.2f;  // Boosted to reach higher
                heights[i] = heights[i] * 0.65f + target * 0.35f;
            }
            queue_draw();
            return true;
        }, 67); // ~15 FPS
    }

private:
    AudioMeter& meter;
    std::vector<float> heights;
    Glib::RefPtr<Gdk::Pixbuf> image;

    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override {
        int width = get_allocation().get_width();
        int height = get_allocation().get_height();
        int bar_width = width / 48;

        cr->set_source_rgba(0, 0, 0, 0);
        cr->paint();

        for (int i = 0; i < 48; ++i) {
            float bar_height = std::max(2.0f, heights[i]);
            int x = i * bar_width;
            int y = height - bar_height;

            // Color gradient based on intensity
            float intensity = bar_height / height;
            if (intensity < 0.3f) {
                cr->set_source_rgba(0.745, 0.788, 0.933, 0.8); // Light blue
            } else if (intensity < 0.6f) {
                cr->set_source_rgba(0.745, 0.788, 0.933, 0.6); // Medium blue
            } else {
                cr->set_source_rgba(0.745, 0.788, 0.933, 1.0); // Deep blue
            }

            cr->rectangle(x, y, bar_width - 12, bar_height);
            cr->fill();

            // Top highlight
            cr->set_source_rgba(1.0, 1.0, 1.0, 0.6);
            cr->rectangle(x, y, bar_width - 12, std::min(3.0f, bar_height));
            cr->fill();

            // Draw image ABOVE bar if there’s space
            if (image && bar_height > 10) {
                int img_size = std::min(bar_width - 12, 40);
                auto scaled = image->scale_simple(img_size, img_size, Gdk::INTERP_NEAREST);
                int img_x = x + (bar_width - img_size) / 2;
                int img_y = y - img_size - 4; // 4px padding

                if (img_y > 0) {
                    Gdk::Cairo::set_source_pixbuf(cr, scaled, img_x, img_y);
                    cr->paint();
                }
            }
        }

        if (!meter.has_audio()) {
            cr->set_source_rgba(1.0, 0.6, 0.8, 0.8);
            cr->select_font_face("sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
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
