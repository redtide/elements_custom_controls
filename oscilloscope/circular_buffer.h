/*=============================================================================
   Copyright (c) 2021 Johann Philippe

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include<vector>
#include<iostream>
#include<memory.h>

using namespace std;

// Circular buffer for audio signals
template<typename T>
class circular_vector
{
public:
    circular_vector() : pos(0), pos_r(0), pos_mark(0)
    {}

    circular_vector(int size, int init = 0) : data(size, init), pos(0), pos_r(0), pos_mark(0) {}

    void set(T value) {
        data[pos] = value;
        pos++;
        if(pos >= data.size()) pos = 0;
    }

    void set(T *vals_ptr, int size)
    {
        for(int i = 0; i < size; ++i) {
            data[pos] = vals_ptr[i];
            pos++;
            if(pos >= data.size()) pos = 0;
        }
    }

    // method for reading values
    int init_read() {
        pos_r = 0;
        pos_mark = pos;
        return pos_r;
    }

    int next() {
        pos_r++;
        return pos_r;
        if(pos_r == pos_mark) {
            // end of reading
            return false;
        }

        return true;
    }

    T get() {
        return data[(pos_r + pos_mark) % data.size()];
    }

    int get_read_index()
    {
        return pos_r + pos_mark;
    }

    int get_absolute_read_index()
    {
        return pos_r;
    }

    T get_at_absolute(int index)
    {
        return data[index];
    }

    T get_at(int index)
    {
        return data[(index + pos_mark) % data.size()];
    }

    T& operator[](int idx) {
        return data[(idx + pos_mark) %data.size()];
    }

    T operator[](int idx) const {
        return data[(idx + pos_mark) %data.size()];
    }


    vector<T> *getData() {
        return &data;
    }

    int size() {
        return data.size();
    }

    void resize(int s) {
        data.resize(s);
    }

    void fill(double val)
    {
        ::memset(data.data(), val, sizeof(T) * data.size());
    }

    // debug
    void print()
    {
        init_read();
        for(bool b = true; b == true; b = next()) {
            cout << get() << " ";

        }
        cout << endl;
        cout << endl;
        cout << endl;
    }
    void print_linear()
    {
        for(auto & it : data) {cout << it << " ";}
        cout << endl;
        cout << endl;
        cout << endl;
    }



private:
    int pos, pos_r, pos_mark;

    vector<T> data;
};

#endif // CIRCULAR_BUFFER_H
