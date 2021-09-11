/*=============================================================================
   Copyright (c) 2021 Johann Philippe

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include"concurrent_buffers.h"
#include<math.h>

    enum waveform {
        sine = 0,
        saw_up = 1,
        saw_down = 2,
        triangle = 3,
        square = 4
    };

template<typename T>
class oscillator
{
public:

    oscillator() : up(true) {}


    oscillator(size_t sr, size_t vec_size, T frequency) :
        sample_rate(sr), vector_size(vec_size),
        phasor(0), buffer(),
        frequency_(frequency), amp_(0.0), triangle_phase(0)
    {}


    void increment_phasor()
    {
        //T incr_per_vector = T(vector_size) / T(sample_rate);
        //T incr = incr_per_vector / T(vector_size) * frequency_;
        double incr = (double(1.0) / double(sample_rate)) * double(frequency_);
        phasor += incr;
        if(up) triangle_phase += incr * 2;

        triangle_phase = abs(( std::fmod(T(phasor) , T(1.0)) ) - 0.5 ) * 2;

        if(phasor >= 1.0) {
            phasor = 0.0;
        }
    }

    void get_sine()
    {
        for(int i = 0; i < vector_size; i++) {
            increment_phasor();
            buffer.push( sin(M_PI * phasor * 2) * amp_);
        }
    }

    void get_saw_down()
    {
        for(int i = 0; i < vector_size; i++) {
            increment_phasor();
            buffer.push( ((phasor * 2.0) - 1.0) * amp_);
        }

    }

    void get_saw_up()
    {
        for(int i = 0; i < vector_size; i++) {
            increment_phasor();
            buffer.push( (((1.0 - phasor) * 2.0) - 1.0) * amp_);
        }

    }

    void get_triangle()
    {
        for(int i = 0; i < vector_size; i++) {
            increment_phasor();
            buffer.push( (2 * (triangle_phase - 0.5)) * amp_);
        }
    }

    void get_square()
    {
        for(int i = 0; i < vector_size; i++) {
            increment_phasor();
            buffer.push( ((phasor < 0.5) ? -1.0 : 1.0) * amp_);
        }
    }


    void update()
    {
        if(pause) {
            buffer.unlock();
            return;
        }
        switch(waveform_)
        {
        case sine:
            get_sine();
            break;
        case saw_up:
            get_saw_up();
            break;
        case saw_down:
            get_saw_down();
            break;
        case triangle:
            get_triangle();
            break;
        case square:
            get_square();
            break;
        }
    }

    concurrent_queue<T>& get_buffer()
    {
        return buffer;
    }

    void set_waveform(waveform w)
    {
        this->waveform_ = w;

    }

    void set_frequency(double freq)
    {
        frequency_ = T(freq);
    }

    void set_amp(double amp)
    {
        amp_ = amp;
    }

    void set_pause(bool b)
    {
        pause = b;
    }

private:
    std::atomic<bool> pause = false;
    waveform waveform_ = sine;
    size_t sample_rate, vector_size;
    T phasor, triangle_phase;
    bool up;
    concurrent_queue<T> buffer;
    T frequency_;
    double amp_;
};


#endif // OSCILLATOR_H
