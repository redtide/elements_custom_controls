#ifndef CURVE_POINT_H
#define CURVE_POINT_H
#include<elements.hpp>

using namespace cycfi::elements;

struct curve_point : public point
{
    curve_point(float x_, float y_, float curve_) : point(x_, y_), curve(curve_)
    {}

    curve_point(point &p, float curve_) : point(p), curve(curve_)
    {}

    template<typename T>
    float distance_to(T &p)
    {
        const float a2 = (p.x - x) * (p.x - x);
        const float b2 = (p.y - y) * (p.y - y);
        const float dist = sqrt(a2 + b2);
        return dist;
    }

   float curve;
};
#endif // CURVE_POINT_H
