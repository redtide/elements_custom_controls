/*=============================================================================
   Copyright (c) 2021 Johann Philippe

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements.hpp>
#include<iostream>
#include<chrono>
#include"spline.h"
#include"curve_point.h"

using namespace cycfi::elements;
using namespace cycfi::artist;

// Main window background color
auto constexpr bkd_color = rgba(35, 35, 37, 255);
auto background = box(bkd_color);


enum curve_mode {
        linear = 0,
        log_exp = 1,
        quadratic_bezier = 2,
        cubic_bezier = 3,
        cubic_spline = 4,
};

struct custom_radio_button_element : toggle_selector, basic_receiver<button_state>
{
    using toggle_selector::toggle_selector;

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

};

inline auto custom_radio_button(std::string text)
{
    return choice(custom_radio_button_element{text});
}

struct curve_editor_controller
{
    curve_editor_controller(curve_mode m = linear, int grain = 2048, int grid_steps_ = 10) : mode(m), granularity(grain), grid_steps(grid_steps_)
    {}
    curve_mode mode;
    int granularity;
    int grid_steps;
};


struct curve_editor_style
{

};

class curve_editor : public tracker<>, receiver<curve_editor_controller>
{
public:

    constexpr static const  float selection_distance = 0.02f;



    curve_editor() : grid_steps(10), granularity(2048), mode(curve_mode::linear)
    {
    }

    float get_curve(float beg, float ending, int dur, int idx, float typ)
    {
      double res;
      double type;
      if(typ == 0) {
        res = beg + (idx * (( ending - beg) / dur));
      }else {
        if(typ > 10) type = 10;
        else if(typ <  -10) type = -10;
        else type = typ;
        res = beg + (ending - beg) * (1 - exp(idx * type / (dur - 1))) / (1-exp(type));
      }
      return res;
    }



    bool key(const context &ctx, key_info k) override
    {
        return false;
    }

    int find_sample(point &p, const context &ctx)
    {
        auto size = ctx.bounds.size();
        auto pos = ctx.bounds.top_left();
        const point relative( (p.x - pos.x) / size.x, (p.y - pos.y) / size.y);

        for(size_t i = 0; i < samples.size(); i++) {
            const float distance = samples[i].distance_to(relative);
            if(distance < selection_distance) {
                return i;
            }
        }
        return -1;
    }

    void add_sample(point &p, const context &ctx)
    {
        auto pos = ctx.bounds.top_left();
        auto size = ctx.bounds.size();
        curve_point cp( (p.x - pos.x) / size.x ,  (p.y - pos.y) / size.y , 0.0);
        if(samples.empty()) {
            samples.push_back(cp);
            selected = 0;
            return;
        }
        if(samples[0].x > cp.x) {
            samples.insert(samples.begin(), cp);
            selected = 0;
            return;
        }

        if(samples.back().x < cp.x) {
            samples.push_back(cp);
            selected = samples.size() - 1;
            return;
        }
        for(size_t i = 0; i < samples.size() - 1; i++) {
            if( samples[i].x < cp.x && samples[i+1].x > cp.x)
            {
                samples.insert(samples.begin() + i + 1, cp);
                selected = i + 1;
                return;
            }
        }
    }

    void draw_segments(const context &ctx)
    {
        if(samples.empty()) return;
        canvas &cnv = ctx.canvas;
        auto size = ctx.bounds.size();
        auto pos = ctx.bounds.top_left();
        cnv.stroke_color(colors::purple);
        cnv.line_width(1.5);
        switch(mode)
        {
        case curve_mode::linear:
        {
            for(size_t i = 0; i < samples.size() - 1; i++) {
                const point p1(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y);
                const point p2(samples[i + 1].x * size.x + pos.x, samples[i + 1].y * size.y + pos.y);
                cnv.move_to(p1);
                cnv.line_to(p2);
                cnv.stroke();
            }
            break;
        }
        case curve_mode::log_exp:
        {
            for(size_t i = 0; i < samples.size() - 1; i++) {
                if(samples[i].curve == 0.0f) {
                    const point p1(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y);
                    const point p2(samples[i + 1].x * size.x + pos.x, samples[i + 1].y * size.y + pos.y);
                    cnv.move_to(p1);
                    cnv.line_to(p2);
                    cnv.stroke();
                } else {
                    const float granularized_begx = samples[i].x * granularity;
                    const float granularized_fbx = samples[i + 1].x * granularity;
                    const float amp_x = abs(granularized_begx - granularized_fbx);
                    point p1(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y);
                    for(int l_idx = 0; l_idx < int(amp_x); l_idx++ ) {
                        const float curve =  (samples[i].y > samples[i + 1].y) ? -samples[i].curve :  samples[i].curve;
                        const float y_val = get_curve(samples[i].y, samples[i + 1].y, int(amp_x), l_idx, curve) * size.y + pos.y;
                        const float x_val = ((l_idx + granularized_begx) / granularity) * size.x + pos.x;
                        cnv.move_to(p1);
                        cnv.line_to(x_val, y_val);
                        cnv.stroke();
                        p1 = point(x_val, y_val);
                    }
                }
            }
            break;
        }
        case curve_mode::cubic_spline:
        {
            if(samples.size() < 3) return;
            const float step = 1.0f / float(granularity) * size.x;
            vector<float> &vec = spline.interpolate_from_points(samples, granularity, size);


            point p1, p2;
            const int start_x = int(samples.front().x * granularity) + 1;
            const int end_x = int(samples.back().x *granularity) - 1;

            for(int i = start_x; i < end_x; i++)
            {
                const float x_relative = float(i) / float(granularity);
                const float x_absolute = ( x_relative )  * size.x + pos.x;
                p1 = point(x_absolute, (vec[i] * size.y) + pos.y);
                p1 = in_bounds(ctx, p1);
                p2 = point(x_absolute + step, vec[i+1] * size.y + pos.y);
                p2 = in_bounds(ctx, p2);

                cnv.move_to(p1);
                cnv.line_to(p2);
                cnv.stroke();
            }
            break;
        }

        case curve_mode::cubic_bezier:
        {
            if(samples.size() < 4) return;
            for(size_t i = 0; i < samples.size() - 3; i+=3) {
                const point p1(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y);
                const point p2(samples[i + 1].x * size.x + pos.x, samples[i + 1].y * size.y + pos.y);
                const point p3(samples[i + 2].x * size.x + pos.x, samples[i + 2].y * size.y + pos.y);
                const point p4(samples[i + 3].x * size.x + pos.x, samples[i + 3].y * size.y + pos.y);
                cnv.move_to(p1);
                cnv.bezier_curve_to(p2, p3, p4);
                cnv.stroke();
            }
            break;

        }
        case curve_mode::quadratic_bezier:
        {
            if(samples.size() < 3) return;
            for(size_t i = 0; i < samples.size() - 2; i+=2) {
                const point p1(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y);
                const point p2(samples[i + 1].x * size.x + pos.x, samples[i + 1].y * size.y + pos.y);
                const point p3(samples[i + 2].x * size.x + pos.x, samples[i + 2].y * size.y + pos.y);
                cnv.move_to(p1);
                cnv.quadratic_curve_to(p2, p3);
                cnv.stroke();
            }

            break;
        }
        default:
            break;
        }
    }

    void draw_samples(const context &ctx)
    {
        if(samples.empty()) return;
        canvas& cnv = ctx.canvas;
        auto size = ctx.bounds.size();
        auto pos = ctx.bounds.top_left();

        cnv.fill_color(colors::red.opacity(0.5));

        for(int i = 0; i < int(samples.size()); i++)
        {
            if(i == focused) {
               cnv.fill_color(colors::red.opacity(0.2));
               cnv.add_circle(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y, 10);
               cnv.fill();
               cnv.fill_color(colors::red);
            }
            if(i == selected) cnv.fill_color(colors::salmon);
            else cnv.fill_color(colors::red.opacity(0.7));
            cnv.add_circle(samples[i].x * size.x + pos.x, samples[i].y * size.y + pos.y, 5);
            cnv.fill();
        }
    }

    void draw_grid(const context &ctx)
    {
        if(!grid_enabled) return;
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

    bool scroll(context const&ctx, point dir, point p) override
    {
        auto size = ctx.bounds.size();
        auto pos = ctx.bounds.top_left();
        const point relative((p.x - pos.x) / size.x, (p.y - pos.y) / size.y);
        for(size_t i = 0; i < samples.size() - 1; i++)
        {
            if(relative.x > samples[i].x && relative.x < samples[i + 1].x) {
                samples[i].curve += (dir.y / 10.0);
                ctx.view.refresh();
                return true;
            }
        }
        return true;
    }

    void draw(const context &ctx) override
    {
        canvas& cnv = ctx.canvas;
        cnv.fill_color(color(0.1, 0.1, 0.1));
        cnv.add_rect(ctx.bounds);
        cnv.fill();
        this->draw_grid(ctx);
        this->draw_segments(ctx);
        this->draw_samples(ctx);
    }

    void layout(const context &ctx) override
    {
    }

    bool cursor(context const &ctx, point p, cursor_tracking status) override
    {
        const int found = find_sample(p, ctx);
        if(found != focused) {
            focused = found;
            ctx.view.refresh();
        }
        last_position = point( (p.x - ctx.bounds.top_left().x) / ctx.bounds.size().x,
                               (p.y - ctx.bounds.top_left().y) / ctx.bounds.size().y);
        return true;
    }

    void snap_to_grid(const context &ctx)
    {
        if(focused == -1) return;
        if(!grid_enabled) return;

        float x_snap = -1.0f, y_snap = -1.0f;

        const float step =  1.0f / float(grid_steps);
        const float step_d2 = step / 2.0f;
         for(size_t i = 0; i < grid_steps + 1; i++) {
             const float relative = (float(i) / float(grid_steps));

             const float x_dist = std::abs(samples[focused].x - relative);
             if(x_dist < (step_d2)) x_snap = relative;

             const float y_dist = std::abs(samples[focused].y - relative);
             if(y_dist < (step_d2)) y_snap = relative;

             if( (x_snap != -1.0f) && (y_snap != -1.0f) ) break;
         }

         samples[focused].x = x_snap;
         samples[focused].y = y_snap;
         if( size_t(focused) < samples.size() - 1 && samples[focused].x > samples[focused + 1].x) {
             std::swap(samples[focused], samples[focused + 1]);
             focused += 1;
         }
         else if( focused > 0 && samples[focused].x < samples[focused - 1].x ) {
             std::swap(samples[focused], samples[focused - 1]);
             focused -= 1;
         }
         ctx.view.refresh();
    }

    bool click(const context &ctx, mouse_button btn) override
    {

        if(btn.down) {
            const int found = find_sample(btn.pos, ctx);
            auto t2 = std::chrono::high_resolution_clock::now();
            const double dur =
                    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - last_click).count();
            if(dur < 200) { // double click (200 ms)
                snap_to_grid(ctx);
            }
            else { // simple click
                last_click = std::chrono::high_resolution_clock::now();
                if(found == -1) { // no sample here
                    add_sample(btn.pos, ctx);
                    focused = selected; // add_sample selects the added sample
                } else { // sample here
                    if( (btn.modifiers == mod_alt || btn.modifiers == mod_shift) && found != -1 ) {
                        samples.erase(samples.begin() + found);
                        selected = -1;
                        focused = -1;
                    } else {
                        selected = found;
                    }
                }
            }
        } else { // btn up
            selected = -1;
        }
        ctx.view.refresh();
        return true;
    }

    void end_tracking(const context &, tracker_info &) override
    {
    }

    point in_bounds(const context &ctx,  point &p)
    {
        auto pos = ctx.bounds.top_left();
        auto size = ctx.bounds.size();
        point pb(p);
        if(pb.x < pos.x ) pb.x = pos.x;
        else if(pb.x > (pos.x + size.x)) pb.x = pos.x + size.x;

        if(pb.y  < pos.y) pb.y = pos.y;
        else if(pb.y > (pos.y + size.y)) pb.y = pos.y + size.y;

        return pb;
    }


    void drag(const context &ctx, mouse_button btn) override
    {
        auto size = ctx.bounds.size();
        auto pos = ctx.bounds.top_left();
        const point btn_pos = in_bounds(ctx, btn.pos);
        const point btn_pos_relative( (btn_pos.x - pos.x) / size.x, (btn_pos.y - pos.y) / size.y );
        if(selected != -1)  {
            samples[selected].x = btn_pos_relative.x;
            samples[selected].y = btn_pos_relative.y;
            if( size_t(selected) < samples.size() - 1 && samples[selected].x > samples[selected + 1].x) {
                std::swap(samples[selected], samples[selected + 1]);
                selected +=1;
                focused += 1;
            }
            else if( selected > 0 && samples[selected].x < samples[selected - 1].x ) {
                std::swap(samples[selected], samples[selected - 1]);
                selected -=1;
                focused -= 1;
            }
            ctx.view.layout();
            //ctx.view.refresh();
        } else if(selected == -1 && (btn.modifiers == mod_alt || btn.modifiers == mod_shift)){ // find segment
            if( (btn_pos_relative.x < samples.front().x) || (btn_pos_relative.x > samples.back().x) ) return;
            for(size_t i = 0; i < samples.size() - 1; i++)
            {
                   if( (btn_pos_relative.x > samples[i].x) && (btn_pos_relative.x < samples[i + 1].x))
                   {
                      samples[i].curve += (last_position.y - btn_pos_relative.y);
                      if(samples[i].curve > 10.0) samples[i].curve = 10.0f;
                      else if(samples[i].curve < -10.0) samples[i].curve = -10.0f;
                      break;
                   }
            }

            if(mode == log_exp) {
                ctx.view.refresh();
            }
        }
    }


    void set_mode(curve_mode c)
    {
        if(c != mode) {
            mode = c;

            auto state = get_state();
        }

        this->mode = c;
    }

    curve_editor_controller value() const override
    {
        return curve_editor_controller(mode, granularity, grid_steps);
    }

    void value(curve_editor_controller val) override
    {
        mode = val.mode;
        grid_steps = val.grid_steps;
        granularity = val.granularity;
    }


private:
    color background_color, grid_color, curve_color, button_color;
    point last_position;
    curves::cubic_spline<float> spline;
    bool grid_enabled = true;
    size_t grid_steps;
    size_t granularity;
    std::chrono::high_resolution_clock::time_point last_click;
    curve_mode mode;
    int selected = -1;
    int focused = -1;
    std::vector<curve_point> samples;

};

class my_app : public app
{

public:
    my_app(int argc, char *argv[]);

private:
    auto make_mode_buttons();
    auto make_help_button();
    auto make_control();
    void make_info_popup();

    window _win;
    view _view;
    curve_editor editor;
};

auto my_app::make_mode_buttons()
{
   auto spline_mode = custom_radio_button("Spline");
   auto log_exp = custom_radio_button("log_exp");
   auto quadbezier = custom_radio_button("quad bezier");
   auto cubbezier = custom_radio_button("cubbezier");
   auto linear = custom_radio_button("linear");

   linear.select(true);
   spline_mode.on_click = [&](bool b) {
       if(!b) return;
       //editor.value(curve_editor_controller(curve_mode::cubic_spline));
       editor.set_mode(curve_mode::cubic_spline);
       _view.refresh();
   };
   log_exp.on_click = [&](bool b) {
       if(!b) return;
       editor.value(curve_editor_controller(curve_mode::log_exp));
       _view.refresh();
   };
   cubbezier.on_click = [&](bool b) {
       if(!b) return;
       editor.value(curve_editor_controller(curve_mode::cubic_bezier));
       _view.refresh();
   };
   quadbezier.on_click = [&](bool b) {
       if(!b) return;
       editor.value(curve_editor_controller(curve_mode::quadratic_bezier));
       _view.refresh();
   };
   linear.on_click = [&](bool b) {
       if(!b) return;
       editor.value(curve_editor_controller(curve_mode::linear));
       _view.refresh();
   };


   return group("curve mode", top_margin(25, margin({10,10,10,10}, htile(
               linear, hspacer(3), log_exp, hspacer(3), spline_mode, hspacer(3),  cubbezier,  hspacer(3), quadbezier
               ))));
}

auto my_app::make_help_button()
{
    auto hb = icon_button(icons::info, 2.0, colors::light_steel_blue);
    hb.on_click = [&](bool b){
      if(b) {
          make_info_popup();
      }
    };
    auto help = align_center( hb);
    return help;
}

void my_app::make_info_popup()
{
    std::string help = "Curve Editor \n"
                       "Click on the canvas and plot samples to draw 2D curves. \n"
                       "There are several modes (displayed at the bottom). Some require 2 samples to work (linear, log_exp), while some require 3 (cubic spline, quadbezier) or 4 (cubbezier). \n"
                       "To curve the log_exp mode you can alt or shift click + drag up / down or simply scroll. \n"
                       "Alt+click or shift+click on a sample removes it.\n";
    auto on_ok = [&](){

    };


    auto popup = message_box1(_view, help, icons::info, on_ok, "OK", point(500, 150) );
    _view.add(popup);
}

auto my_app::make_control()
{
    return  htile(hstretch(1900, vstretch(1900, (link(editor)))));
}

my_app::my_app(int argc, char *argv[]) :
    app(argc, argv, "Curve Editor", "com.johannphilippe.curve_editor"),
    _win(name()), _view(_win)
{
    _win.on_close = [this](){
      this->stop();
    };

    _win.position(point(600, 400));


    _view.content(
                margin({15, 15, 15, 15},
                       hstretch(1,
                       htile(
                       vtile(
                           make_help_button(),
                           top_margin(15, make_control()),
                           //top_margin(15, make_mode_buttons())
                           top_margin(15, make_mode_buttons())
                           )))
                       ),
                background
                );
}


int main(int argc, char* argv[])
{
    my_app _app(argc, argv);
    _app.run();
   return 0;
}
