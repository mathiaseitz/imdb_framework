#ifndef DESCRIPTOR_TOOLS_H
#define DESCRIPTOR_TOOLS_H

#include <QPoint>
#include <QImage>

namespace descriptor_tools {


    const int RIGHT = 8;  //1000
    const int TOP = 4;    //0100
    const int LEFT = 2;   //0010
    const int BOTTOM = 1; //0001
    typedef int outcode;

    void extractSketch(const QImage& img, bool is_sketch, image<int>& sketchImg);
    void extractEdgeLength(const image<int>& sketchImg, image<float> & edgeLengthImg);
    void extractFilteredSketch(const QImage& img, bool is_sketch, image<int>& sketchImg);
    //void computeOrientationImage()

    // angle is in radians
    QPoint getBorderIntersection(const QPoint& coordinate, float angle, int w, int h);


    bool traceRay(const QPoint& coordinate, const float angle, const image<int>& cannyImg, QPoint& hitPoint, float maxLength);

    // adapted from http://www.google.com/codesearch/p?hl=en#P6vefBNJpH4/courses/cse40166/www/BOOK_PROGRAMS/bresenham.c&q=bresenham&sa=N&cd=2&ct=rc&d=3
    bool bresenham(int x1,int y1,int x2,int y2, const image<int> & cannyImg, int & intersect_x, int & intersect_y);

    //Compute the bit code for a point (x, y) using the clip rectangle
    //bounded diagonally by (xmin, ymin), and (xmax, ymax)
    outcode computeOutCode (double x, double y, int xmin, int ymin, int xmax, int ymax);

    // implementation taken from:
    // http://en.wikipedia.org/wiki/Cohen-Sutherland
    //Cohen-Sutherland clipping algorithm clips a line from
    //P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with
    //diagonal from (xmin, ymin) to (xmax, ymax).
    bool cohenSutherland(double& x0, double& y0,double& x1, double& y1, int xmin, int ymin, int xmax, int ymax);

    template <class descr_t>
    void extractWords(const descr_t& descr, std::vector<typename descr_t::feature_t::word_t>& words) {
        words.clear();
        words.reserve(descr.feature_vector.size());
        for (std::size_t i = 0; i < descr.feature_vector.size(); i++) {
            words.push_back(descr.feature_vector[i].word);
        }
    }

};
#endif // DESCRIPTOR_TOOLS_H
