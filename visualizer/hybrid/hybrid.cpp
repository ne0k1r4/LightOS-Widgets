#include <gtkmm.h>
#include <gdkmm/general.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace {
constexpr int FFTN=1024, BARS=72, H=145;
constexpr double PI=3.14159265358979323846;
struct RGB { double r,g,b; };
struct Palette { RGB accent{0.90,0.45,0.75}, soft{0.98,0.72,0.91}, pale{1,0.91,0.99}, deep{0.16,0.08,0.26}; };
RGB fromhex(const std::string& s,RGB fallback){
 if(s.size()!=7 || s[0]!='#')return fallback;
 try { auto v=std::stoul(s.substr(1),nullptr,16);return {((v>>16)&255)/255.,((v>>8)&255)/255.,(v&255)/255.};}catch(...){return fallback;}
}
Palette load_palette(){
 Palette p; const char* home=std::getenv("HOME");if(!home)return p;
 std::ifstream f(std::string(home)+"/.config/waybar/wallpaper-colors.css");std::string key,value;
 while(f>>key>>value){if(key!="@define-color")continue;std::string color;f>>color;
  if(value=="wallpaper_accent")p.accent=fromhex(color,p.accent);
  else if(value=="wallpaper_accent_soft")p.soft=fromhex(color,p.soft);
  else if(value=="wallpaper_accent_pale")p.pale=fromhex(color,p.pale);
  else if(value=="wallpaper_accent_deep")p.deep=fromhex(color,p.deep);
 }
 return p;
}
void fft(std::array<std::complex<double>,FFTN>& a){
 for(int i=1,j=0;i<FFTN;i++){int bit=FFTN>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
 for(int len=2;len<=FFTN;len<<=1){double ang=-2*PI/len;std::complex<double>wlen(std::cos(ang),std::sin(ang));
  for(int i=0;i<FFTN;i+=len){std::complex<double>w(1);for(int j=0;j<len/2;j++){auto u=a[i+j],v=a[i+j+len/2]*w;a[i+j]=u+v;a[i+j+len/2]=u-v;w*=wlen;}}}
}
class Capture {
public:
 std::array<float,BARS> bins{};std::array<float,256> wave{};float bass=0;
 std::mutex mutex;std::atomic<bool> running{true};std::thread worker;
 Capture(){worker=std::thread([this]{loop();});}
 ~Capture(){running=false;if(worker.joinable())worker.join();}
 void loop(){
  const char* monitor=std::getenv("LIGHTOS_AUDIO_MONITOR");
  std::string detected;
  if(!monitor || !*monitor){
    if(FILE* pipe=popen("pactl get-default-sink 2>/dev/null", "r")){
      char buf[256]{};
      if(fgets(buf,sizeof(buf),pipe)){detected=buf;detected.erase(detected.find_last_not_of("\r\n")+1);}
      pclose(pipe);
      if(!detected.empty()){detected+=".monitor";monitor=detected.c_str();}
    }
  }
  pa_sample_spec spec{PA_SAMPLE_FLOAT32LE,44100,2};int error=0;
  pa_simple* stream=pa_simple_new(nullptr,"LightOS Hybrid Visualizer",PA_STREAM_RECORD,(monitor&&*monitor)?monitor:nullptr,"desktop spectrum",&spec,nullptr,nullptr,&error);
  if(!stream){std::cerr<<"PulseAudio capture: "<<pa_strerror(error)<<" (check LIGHTOS_AUDIO_MONITOR)\n";return;}
  std::array<float,FFTN*2> raw{};std::array<std::complex<double>,FFTN> data{};
  while(running){if(pa_simple_read(stream,raw.data(),sizeof(raw),&error)<0){std::cerr<<"Audio read: "<<pa_strerror(error)<<"\n";break;}
   double rms=0;for(int i=0;i<FFTN;i++){double x=(raw[i*2]+raw[i*2+1])*0.5;rms+=x*x;data[i]=x*(0.5-0.5*std::cos(2*PI*i/(FFTN-1)));}
   std::array<float,BARS> next{};
   fft(data);
   for(int k=0;k<BARS;k++){
    double f0=45*std::pow(16000./45.,k/double(BARS)),f1=45*std::pow(16000./45.,(k+1)/double(BARS));
    int lo=std::clamp(int(f0*FFTN/44100),1,FFTN/2-1),hi=std::clamp(int(f1*FFTN/44100)+1,lo+1,FFTN/2);
    double maximum=0;for(int i=lo;i<hi;i++)maximum=std::max(maximum,std::abs(data[i]));
    next[k]=std::clamp(float(std::log1p(maximum*0.45)/3.3),0.f,1.f);
   }
   std::lock_guard<std::mutex> lock(mutex);
   for(int k=0;k<BARS;k++)bins[k]=std::max(next[k],bins[k]*0.77f); // fast attack, slower release
   for(int i=0;i<256;i++)wave[i]=std::clamp(raw[i*8]*2.0f,-1.f,1.f);
   float target=std::clamp(float(std::sqrt(rms/FFTN)*3.0),0.f,1.f);
   bass=std::max(target,bass*0.80f);
  }
  pa_simple_free(stream);
 }
};
struct Particle {double x=0,y=0,vy=0,life=0;};
class Visual : public Gtk::DrawingArea {
 Capture& audio;Palette palette;std::array<float,BARS> bars{};std::array<float,256> waveform{};
 std::vector<Particle> particles;std::mt19937 rng{42};float energy=0;unsigned tick=0; double beat=0, prevEnergy=0; std::array<double,4> spring{}, velocity{};
 // LIGHTOS_FLOATING_ART_V2: original character assets, no new artwork.
 Glib::RefPtr<Gdk::Pixbuf> image_a, image_b;
 void load_art(){
  const char* config=g_get_user_config_dir();
  std::string dir=std::string(config?config:"")+"/Light/assets/settings/";
  try { image_a=Gdk::Pixbuf::create_from_file(dir+"elyfly.png"); } catch(const Glib::Error& e) { std::cerr<<"Optional art elyfly.png: "<<e.what()<<"\n"; }
  try { image_b=Gdk::Pixbuf::create_from_file(dir+"elyhoc.png"); } catch(const Glib::Error& e) { std::cerr<<"Optional art elyhoc.png: "<<e.what()<<"\n"; }
 }
 static RGB luminous(RGB accent,RGB soft){
  double brightness=0.2126*accent.r+0.7152*accent.g+0.0722*accent.b;
  // Preserve wallpaper hue, but blend in the soft accent for legibility.
  double blend=brightness<0.18?0.78:(brightness<0.35?0.52:0.24);
  return {accent.r*(1-blend)+soft.r*blend,accent.g*(1-blend)+soft.g*blend,accent.b*(1-blend)+soft.b*blend};
 }
 void color(const Cairo::RefPtr<Cairo::Context>&cr,RGB c,double alpha){cr->set_source_rgba(c.r,c.g,c.b,alpha);}
public:
 explicit Visual(Capture&a):audio(a){set_size_request(-1,H);palette=load_palette();load_art();
  Glib::signal_timeout().connect([this]{std::lock_guard<std::mutex> l(audio.mutex);bars=audio.bins;waveform=audio.wave;energy=audio.bass;update_particles();queue_draw();return true;},16);
  Glib::signal_timeout().connect([this]{palette=load_palette();return true;},2000);
 }
 void update_particles(){
  for(auto& p:particles){p.y+=p.vy;p.life-=0.025;}
  particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle&p){return p.life<=0;}),particles.end());
  if(energy>0.14f && particles.size()<95){int n=energy>0.55?3:1;std::uniform_real_distribution<double> d(0,1);
   for(int i=0;i<n;i++)particles.push_back({d(rng),H-12,-(0.7+energy*2.3+d(rng)),0.5+d(rng)*0.5});}
  // Bass transients excite damped springs; motion settles gracefully in silence.
  double low=0;for(int k=0;k<10;k++)low+=bars[k];low/=10.0;
  double impulse=std::max(0.0,low-prevEnergy*0.88);
  prevEnergy=prevEnergy*0.84+low*0.16;
  beat=std::max(beat*0.86,std::min(1.0,impulse*5.5));
  for(int i=0;i<4;i++){
    double target=std::min(1.0,low*0.8+beat*(0.7+0.08*i));
    velocity[i]+=(target-spring[i])*0.11;
    velocity[i]*=0.79;
    spring[i]=std::clamp(spring[i]+velocity[i],0.0,1.5);
  }
  tick++;
 }
 bool on_draw(const Cairo::RefPtr<Cairo::Context>&cr) override {
  int w=get_allocated_width();if(w<=0)return true;
  cr->set_operator(Cairo::OPERATOR_SOURCE);cr->set_source_rgba(0,0,0,0);cr->paint();cr->set_operator(Cairo::OPERATOR_OVER);
  // faint HUD grid and lower baseline
  for(int y=H-20;y>20;y-=26){color(cr,palette.soft,0.055);cr->set_line_width(0.7);cr->move_to(0,y);cr->line_to(w,y);cr->stroke();}
  color(cr,palette.soft,0.30);cr->set_line_width(1.0);cr->move_to(0,H-3);cr->line_to(w,H-3);cr->stroke();
  RGB glow=luminous(palette.accent,palette.soft);
  double bw=w/double(BARS);
  for(int i=0;i<BARS;i++){
   double x=(i+0.12)*bw,amp=std::clamp(double(bars[i]),0.,1.),bh=2+amp*105;
   auto grad=Cairo::LinearGradient::create(x,H-5-bh,x,H-5);
   grad->add_color_stop_rgba(0,palette.pale.r,palette.pale.g,palette.pale.b,0.9*amp);
   grad->add_color_stop_rgba(0.4,palette.soft.r,palette.soft.g,palette.soft.b,0.66*amp);
   grad->add_color_stop_rgba(1,glow.r,glow.g,glow.b,0.42*amp);
   cr->rectangle(x,H-5-bh,std::max(1.,bw*0.72),bh);cr->set_source(grad);cr->fill();
   if(amp>0.12){color(cr,glow,0.30*amp);cr->set_line_width(std::max(2.,bw*1.3));cr->move_to(x+bw*.4,H-5-bh);cr->line_to(x+bw*.4,H-5);cr->stroke();}
  }
  // two ribbons: true PCM waveform plus slow luminous bass envelope
  for(int pass=0;pass<2;pass++){
   cr->begin_new_path();for(int i=0;i<256;i++){
    double x=w*i/255.,v=waveform[i],y=H-22-v*(13+energy*24);
    if(pass==1)y=H-13-(0.45*std::sin(i*.075+tick*.05)+0.55*energy)*14;
    if(i==0)cr->move_to(x,y);else cr->line_to(x,y);
   }
   color(cr,pass==0?palette.pale:palette.soft,pass==0?0.68:0.26);cr->set_line_width(pass==0?1.6:3.5);cr->stroke();
  }
  for(const auto&p:particles){color(cr,palette.pale,p.life*0.85);cr->arc(p.x*w,p.y,1.1+energy*1.8,0,2*PI);cr->fill();}
  // Original LightOS sprites float above audio-reactive band peaks.
  // Four sprites across the desktop; they stay within the 145px overlay.
  for(int slot=0;slot<4;slot++){
   const auto& src=(slot%2==0)?image_a:image_b;
   if(!src)continue;
   int index=std::clamp((slot*2+1)*BARS/9,0,BARS-1);
   double amplitude=std::clamp(double(bars[index]),0.,1.);
   int size=std::clamp(int(24+amplitude*9+spring[slot]*3),24,36);
   double x=w*(slot+0.5)/4.;
   double bob=std::sin(tick*0.045+slot*1.6)*(2.4+energy*2.0)-spring[slot]*22.0;
   double y=std::clamp(double(H-28-size-amplitude*38+bob),2.0,double(H-size-4));
   auto scaled=src->scale_simple(size,size,Gdk::INTERP_BILINEAR);
   if(!scaled)continue;
   cr->save();
   Gdk::Cairo::set_source_pixbuf(cr,scaled,x-size*0.5,y);
   cr->paint_with_alpha(std::clamp(0.78+0.13*amplitude+0.06*spring[slot],0.0,1.0));
   cr->restore();
  }
  return true;
 }
};
class App:public Gtk::Application {
public:App():Gtk::Application("org.lightos.HybridVisualizer"){}
protected:void on_activate() override {
 auto* win=new Gtk::Window();win->set_default_size(-1,H);win->set_decorated(false);win->set_accept_focus(false);win->set_app_paintable(true);
 auto screen=win->get_screen();if(screen){auto v=screen->get_rgba_visual();if(v)gtk_widget_set_visual(GTK_WIDGET(win->gobj()),v->gobj());}
 GtkWindow* g=GTK_WINDOW(win->gobj());gtk_layer_init_for_window(g);gtk_layer_set_layer(g,GTK_LAYER_SHELL_LAYER_BOTTOM);
 gtk_layer_set_anchor(g,GTK_LAYER_SHELL_EDGE_BOTTOM,true);gtk_layer_set_anchor(g,GTK_LAYER_SHELL_EDGE_LEFT,true);gtk_layer_set_anchor(g,GTK_LAYER_SHELL_EDGE_RIGHT,true);
 gtk_layer_set_exclusive_zone(g,0);gtk_layer_set_keyboard_mode(g,GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
 auto* visual=new Visual(capture);win->add(*visual);add_window(*win);win->show_all();}
private:Capture capture;
};
}
int main(int argc,char**argv){auto app=Glib::RefPtr<App>(new App());return app->run(argc,argv);}
