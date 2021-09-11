/*=============================================================================
   Copyright (c) 2021 Johann Philippe

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements.hpp>
#include"concurrent_buffers.h"
#include"circular_buffer.h"
#include<thread>
#include"oscillator.h"
#include<atomic>

using namespace cycfi::elements;
using namespace cycfi::artist;

// Main window background color
auto constexpr bkd_color = rgba(35, 35, 37, 255);
auto background = box(bkd_color);

struct custom_radio_button_element : element, basic_receiver<button_state>
{

    custom_radio_button_element(std::string text)
     : _text(std::move(text))
    {}
    const color background = colors::light_salmon;
    const color hilite_color = colors::light_coral;

    void draw(const context &ctx) override
    {
      auto  state = value();
      auto  value = state.value;
      auto  hilite = state.hilite;
      auto  tracking = state.tracking;
      if(value)
      {
          ctx.canvas.fill_color(background.opacity(0.7));
      } else if(tracking) {
          ctx.canvas.fill_color(background);
      } else if(hilite) {
          ctx.canvas.fill_color(hilite_color);
      } else {
          ctx.canvas.fill_color(background.opacity(0.5));
      }

      ctx.canvas.add_rect(ctx.bounds);
      ctx.canvas.fill();
      rect        box = ctx.bounds.move(15, 0);
      float cx = box.left + 10;
      float cy = ctx.bounds.top + (ctx.bounds.height() / 2);
      ctx.canvas.text_align(ctx.canvas.left | ctx.canvas.middle);

      ctx.canvas.fill_color(colors::ivory);
      ctx.canvas.fill_text(_text.c_str(), point{ cx, cy });
    }

    view_limits limits(const basic_context &ctx) const override
    {
        auto t_size = measure_text(ctx.canvas, _text, get_theme().text_box_font);
        view_limits limits;
        limits.max = point(full_extent, t_size.y + 15);
        return limits;
    }


    std::string _text;
};

inline auto custom_radio_button(std::string text)
{
    return choice(custom_radio_button_element{text});
}



template <typename E>
auto decorate(E&& e)
{
   return  align_center(margin({ 25, 5, 25, 5 },
      std::forward<E>(e)
   ));
}


auto make_label(std::string text = "")
{
   return decorate(heading(text)
      .font_color(get_theme().indicator_hilite_color)
      .font_size(24)
   );
};


auto make_thumbwheel2(char const* unit, float offset, float scale, int precision, std::function<void(double)> f)
{
   auto label = make_label();

   auto&& as_string =
      [=](double val)
      {
        double scaled = (val * scale) + offset;
         std::ostringstream out;
         out.precision(precision);
         out << std::fixed << scaled << unit;
         f(scaled);
         return out.str();
      };

   auto tw = share(thumbwheel(as_label<double>(as_string, label)));

   return top_margin(20,
      layer(
         hold(tw),
         frame{}
      )
   );
}

constexpr const int vector_size = 2048;
constexpr const int sample_rate = 48000;
constexpr const int circular_size = 2048;

class oscilloscope : public tracker<element>
{
public:
    oscilloscope() :
        tracker(), internal_buffer(vector_size, 0),
        freq(1.0), osc(sample_rate, vector_size, freq),
        is_running(false), circular(circular_size, 0)
    {
    }

    void set_waveform(waveform w)
    {
        osc.set_waveform(w);
    }


    void draw_grid(const context &ctx)
    {
        canvas& cnv = ctx.canvas;
                auto size = ctx.bounds.size();
                auto pos = ctx.bounds.top_left();
                cnv.line_width(0.5);
                cnv.stroke_color(colors::ivory.opacity(0.3));
                for(size_t i = 0; i < grid_steps + 1; i++)
                {
                    const float y = (pos.y + (float(i / 10.0f) * size.y));
                    point p1(pos.x, y);
                    point p2(pos.x + size.x, y);
                    path p(rect(p1, p2));
                    cnv.add_path(p);
                    cnv.stroke();


                    const float x = (pos.x + (float(i / 10.0f) * size.x));
                    p1 = point(x, pos.y);
                    p2 = point(x, pos.y + size.y);
                    p = path(rect(p1, p2)) ;
                    cnv.add_path(p);

                    cnv.stroke();
                }
    }

    void draw_scope(const context &ctx)
    {
        if(!is_running) return;

        if(lock_sync) {

        }  else {

            concurrent_queue<float>& cv = osc.get_buffer();
            std::cout << "queue size : " << cv.size() << std::endl;
            float v = 0;
            while(cv.try_pop(v)) {
                circular.set(v);
            }


            for(auto it = circular.init_read(); it < circular.size() - 1; it = circular.next())
            {
                float v = circular.get();
                float vnext = circular.get_at(it + 1);

                const float px1 = float(it) / float(circular_size);
                const float px2 = float(it + 1) / float(circular_size);
                const point p1 = point(px1 * ctx.bounds.size().x + ctx.bounds.left,
                                       ((v + 1) / 2.0) * ctx.bounds.size().y + ctx.bounds.top);
                const point p2 = point(px2 * ctx.bounds.size().x + ctx.bounds.left,
                                       ((vnext + 1) / 2.0) * ctx.bounds.size().y + ctx.bounds.top);
                ctx.canvas.stroke_color(colors::azure);
                ctx.canvas.line_width(2);
                ctx.canvas.move_to(p1);
                ctx.canvas.line_to(p2);
                ctx.canvas.stroke();

            }
        }
    }

    void draw(const context &ctx) override
    {
        ctx.canvas.fill_color(color(0.1, 0.1, 0.1));
        ctx.canvas.add_rect(ctx.bounds);
        ctx.canvas.fill();
        draw_grid(ctx);
        draw_scope(ctx);
    }

    void set_frequency(double freq)
    {
        osc.set_frequency(freq);
    }

    void set_amp(double amp)
    {
        osc.set_amp(amp);
    }

    void run(bool b)
    {
        if(!is_running) {
            is_running = b;
            osc.set_waveform(waveform::sine);
            t = std::thread([&](){
                int time_ms = 0;
                int time_s = 0;
                while(is_running)
                {
                    auto t1 = std::chrono::high_resolution_clock::now();
                    const double sleep_time = 1000.0 / (float(sample_rate) / float(vector_size));
                    osc.update();
                    std::this_thread::sleep_for(std::chrono::milliseconds( int(sleep_time) ));
                    auto t2 = std::chrono::high_resolution_clock::now();
                    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
                    time_ms += dur;
                    int t_s = time_ms / 1000;
                    if(t_s != time_s) std::cout << "time seconds : " << t_s << std::endl;
                    time_s = t_s;
                }
            });
        t.detach();
        } else {
            is_running = b;
        }

    }

    void pause(bool b)
    {
       osc.set_pause(b);
    }

private:
    vector<float> internal_buffer;

    bool lock_sync = false;
    float freq;
    oscillator<float> osc;
    size_t grid_steps = 10;
    std::thread t;
    std::atomic<bool> is_running;
    circular_vector<float> circular;

};

constexpr auto fps = 1000ms / 30;

void animate(view& view_)
{
   view_.post(fps, [&](){animate(view_);});
   view_.refresh();
}


int main(int argc, char* argv[])
{

   app _app(argc, argv, "Oscilloscope", "com.johannphilippe.oscilloscope");
   window _win(_app.name());
   _win.on_close = [&_app]() { _app.stop(); };

   view view_(_win);


   auto osc = oscilloscope();

   auto sine = custom_radio_button("sine");
   auto saw_up = custom_radio_button("saw_up");
   auto saw_down = custom_radio_button("saw_down");
   auto triangle = custom_radio_button("triangle");
   auto square = custom_radio_button("square");

   sine.on_click = [&](bool b) {
       if(b) {
           osc.set_waveform(waveform::sine);
       }
   };
   saw_up.on_click = [&](bool b) {
       if(b) {
           osc.set_waveform(waveform::saw_up);
       }
   };
   saw_down.on_click = [&](bool b) {
       if(b) {
           osc.set_waveform(waveform::saw_down);
       }
   };
   triangle.on_click = [&](bool b) {
       if(b) {
           osc.set_waveform(waveform::triangle);
       }
   };
   square.on_click = [&](bool b) {
       if(b) {
           osc.set_waveform(waveform::square);
       }
   };

   sine.select(true);

   auto freq = make_thumbwheel2(" Hz", 1.0, 499.0, 1, [&](double val) {
       osc.set_frequency(val);
   });

   auto amp = make_thumbwheel2(" amp", 0.0, 1.0, 3, [&](double val) {
      osc.set_amp(val);
   });

   auto tog = toggle_button("run", 1.0, colors::red);
   tog.select(false);
   tog.on_click = [&](bool b)
   {
       osc.run(b);
   };

   auto pause_toggle = toggle_button("pause", 1.0, colors::cyan);
   pause_toggle.select(false);
   pause_toggle.on_click = [&](bool b)
   {
       osc.pause(b);
   };

   view_.post(fps, [&](){animate(view_);});

   view_.content(
               max_size({1900, 1000},
           margin({10, 10, 10, 10},
                  hstretch(2,
                           vstretch(2,
                                    vtile(
                                        link(osc),
                                        top_margin(15, group("Waveform", top_margin(35, htile(sine, saw_up, saw_down, triangle, square)))),
                                         hstretch(2, htile(freq,amp)),
                                        top_margin(15,tog),
                                        top_margin(15, pause_toggle)
                                        )
                                    )
                           )
                  )),
      background
   );

   _app.run();
   return 0;
}
