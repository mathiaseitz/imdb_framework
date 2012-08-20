#include "descriptor_tools.h"

#include <cmath>
#include <cassert>

#include <QRect>

namespace descriptor_tools {

    void extractEdgeLength(const image<int>& sketchImg, image<float> & edgeLengthImg) {
        image<int> resultEdgeLengthImg(sketchImg.width(),sketchImg.height(), 0);

        // first compute edge length - then stretch/normalize the edgelength between 0/255
        computeEdgeLength(sketchImg, resultEdgeLengthImg);
        copy_image(resultEdgeLengthImg, edgeLengthImg, int_to_float());
        stretch(edgeLengthImg, 0, 255);
    }

    // image -> sketch conversion + check if this is necessary
    // the result is stored in sketchImg, which is a binary image
    // with white sketch lines and black background
    void extractFilteredSketch(const QImage& img, bool is_sketch, image<int>& sketchImg) {

        assert(sketchImg.width() == static_cast<size_t>(img.width()) && sketchImg.height() == static_cast<size_t>(img.height()));

        // if the input image is not yet a binary sketch,
        // we first perform our image -> sketch conversion,
        // i.e. we extract canny lines
        if (!is_sketch) {
            image<int> resultCanny(sketchImg.width(), sketchImg.height());
            image<float> imgGrayscale(sketchImg.width(), sketchImg.height());
            copy_qt_image(img, imgGrayscale, qrgb_to_gray<float>());
            canny(imgGrayscale, resultCanny, 5.0, 0.05, 0.15);
            computeEdgeLength(resultCanny, sketchImg);
        }

        // otherwise we can directly use the sketched image, but make sure to
        // ###!!!convert it to the same format as the canny image!!!###,
        // i.e. sketch lines are white, background is black
        else {
            copy_qt_image(img, sketchImg, qrgb_to_binary_inverted());
        }
    }


    // image -> sketch conversion + check if this is necessary
    // the result is stored in sketchImg, which is a binary image
    // with white sketch lines and black background
    void extractSketch(const QImage& img, bool is_sketch, image<int>& sketchImg) {

        assert(sketchImg.width() == static_cast<size_t>(img.width()) && sketchImg.height() == static_cast<size_t>(img.height()));

        // if the input image is not yet a binary sketch,
        // we first perform our image -> sketch conversion,
        // i.e. we extract canny lines
        if (!is_sketch) {
            image<float> imgGrayscale(sketchImg.width(), sketchImg.height());
            copy_qt_image(img, imgGrayscale, qrgb_to_gray<float>());
            canny(imgGrayscale, sketchImg, 5.0, 0.05, 0.2);
        }

        // otherwise we can directly use the sketched image, but make sure to
        // ###!!!convert it to the same format as the canny image!!!###,
        // i.e. sketch lines are white, background is black
        else {
            copy_qt_image(img, sketchImg, qrgb_to_binary_inverted());
        }
    }

    // angle is in radians
    QPoint getBorderIntersection(const QPoint& coordinate, float angle, int w, int h)
    {

        float radius = std::sqrt(w*w + h*h);

        double x1 = coordinate.x() + std::cos(angle)*radius;
        double y1 = coordinate.y() + std::sin(angle)*radius;

        double x0 = coordinate.x();
        double y0 = coordinate.y();

        bool accept = cohenSutherland(x0, y0, x1, y1, 0, 0, w-1, h-1);

        assert(accept);
        assert(x0 == coordinate.x());
        assert(y0 == coordinate.y());

        return QPoint(x1,y1);
    }



    // adapted from http://www.google.com/codesearch/p?hl=en#P6vefBNJpH4/courses/cse40166/www/BOOK_PROGRAMS/bresenham.c&q=bresenham&sa=N&cd=2&ct=rc&d=3
    bool bresenham(int x1,int y1,int x2,int y2, const image<int> & cannyImg, int & intersect_x, int & intersect_y)
    {
        intersect_x = -1;
        intersect_y = -1;
        int dx, dy, i, e;
        int incx, incy, inc1, inc2;
        int x,y;

        dx = x2 - x1;
        dy = y2 - y1;

        if(dx < 0) dx = -dx;
        if(dy < 0) dy = -dy;
        incx = 1;
        if(x2 < x1) incx = -1;
        incy = 1;
        if(y2 < y1) incy = -1;
        x=x1;
        y=y1;

        if(dx > dy)
        {
            if( cannyImg(x,y) ) {
                intersect_x = x;
                intersect_y = y;
                return true;
            }
            e = 2*dy - dx;
            inc1 = 2*( dy -dx);
            inc2 = 2*dy;
            for(i = 0; i < dx; i++)
            {
                if(e >= 0)
                {
                    y += incy;
                    e += inc1;
                }
                else e += inc2;
                x += incx;
                if( cannyImg(x,y) ) {
                    intersect_x = x;
                    intersect_y = y;
                    return true;
                }

            }
        }
        else
        {
            if( cannyImg(x,y) ) {
                intersect_x = x;
                intersect_y = y;
                return true;
            }
            e = 2*dx - dy;
            inc1 = 2*( dx - dy);
            inc2 = 2*dx;
            for(i = 0; i < dy; i++)
            {
                if(e >= 0)
                {
                    x += incx;
                    e += inc1;
                }
                else e += inc2;
                y += incy;
                if( cannyImg(x,y) ) {
                    intersect_x = x;
                    intersect_y = y;
                    return true;
                }
            }
        }
        return false;
    }



    //Compute the bit code for a point (x, y) using the clip rectangle
    //bounded diagonally by (xmin, ymin), and (xmax, ymax)
    outcode computeOutCode (double x, double y, int xmin, int ymin, int xmax, int ymax)
    {
        outcode code = 0;
        if (y > ymax)              //above the clip window
            code |= TOP;
        else if (y < ymin)         //below the clip window
            code |= BOTTOM;
        if (x > xmax)              //to the right of clip window
            code |= RIGHT;
        else if (x < xmin)         //to the left of clip window
            code |= LEFT;
        return code;
    }


    // implementation taken from:
    // http://en.wikipedia.org/wiki/Cohen-Sutherland
    //Cohen-Sutherland clipping algorithm clips a line from
    //P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with
    //diagonal from (xmin, ymin) to (xmax, ymax).
    bool cohenSutherland(double& x0, double& y0,double& x1, double& y1, int xmin, int ymin, int xmax, int ymax)
    {
        //Outcodes for P0, P1, and whatever point lies outside the clip rectangle
        outcode outcode0, outcode1, outcodeOut;
        bool accept = false, done = false;

        //compute outcodes
        outcode0 = computeOutCode (x0, y0, xmin, ymin, xmax, ymax);
        outcode1 = computeOutCode (x1, y1, xmin, ymin, xmax, ymax);

        do{
            if (!(outcode0 | outcode1))      //logical or is 0. Trivially accept and get out of loop
            {
                accept = true;
                done = true;
            }
            else if (outcode0 & outcode1)//logical and is not 0. Trivially reject and get out of loop
            {
                done = true;
            }

            else
            {
                //failed both tests, so calculate the line segment to clip
                //from an outside point to an intersection with clip edge
                double x=-1; // random intialization to avoid compiler warnings
                double y=-1;
                //At least one endpoint is outside the clip rectangle; pick it.
                outcodeOut = outcode0? outcode0: outcode1;
                //Now find the intersection point;
                //use formulas y = y0 + slope * (x - x0), x = x0 + (1/slope)* (y - y0)
                if (outcodeOut & TOP)          //point is above the clip rectangle
                {
                    x = x0 + (x1 - x0) * (ymax - y0)/(y1 - y0);
                    y = ymax;
                }
                else if (outcodeOut & BOTTOM)  //point is below the clip rectangle
                {
                    x = x0 + (x1 - x0) * (ymin - y0)/(y1 - y0);
                    y = ymin;
                }
                else if (outcodeOut & RIGHT)   //point is to the right of clip rectangle
                {
                    y = y0 + (y1 - y0) * (xmax - x0)/(x1 - x0);
                    x = xmax;
                }
                else if (outcodeOut & LEFT)                         //point is to the left of clip rectangle
                {
                    y = y0 + (y1 - y0) * (xmin - x0)/(x1 - x0);
                    x = xmin;
                }
                //Now we move outside point to intersection point to clip
                //and get ready for next pass.
                if (outcodeOut == outcode0)
                {
                    x0 = x;
                    y0 = y;
                    outcode0 = computeOutCode (x0, y0, xmin, ymin, xmax, ymax);
                }
                else
                {
                    x1 = x;
                    y1 = y;
                    outcode1 = computeOutCode (x1, y1, xmin, ymin, xmax, ymax);
                }
            }
        }while (!done);

        return accept;
    }

    bool traceRay(const QPoint& startPoint, const float angle, const image<int>& cannyImg, QPoint& hitPoint, float maxLength)
    {
        uint w = cannyImg.width();
        uint h = cannyImg.height();

        // compute intersection point of ray with the borders of the image,
        // this gives us the endpoint of the line segment which we test for
        // intersection with the canny lines
        QPoint intersection = getBorderIntersection(startPoint, angle, w, h);
        assert(QRect(0,0,w,h).contains(intersection));

        // initialize the hitpoint with a point that is outside our window
        // and run the bresenham algorithm which traces the line segment and
        // returns the first point of that line segment, that lies on a sketch
        // point, if any
        hitPoint.rx() = -1; hitPoint.ry() = -1;
        bool intersected = bresenham(startPoint.x(), startPoint.y(), intersection.x(), intersection.y(), cannyImg, hitPoint.rx(), hitPoint.ry());

        // sanity check: if we got an intersection point it must lie within the image rectangle
        if ( intersected ) assert(QRect(0,0,w,h).contains(hitPoint));

        if( intersected && maxLength > 0 ) {
            int dx = hitPoint.x() - startPoint.x();
            int dy = hitPoint.y() - startPoint.y();
            float len = std::sqrt(dx*dx + dy*dy);

            // if the intersection is outside the localized region
            // we discard the hitpoint again
            if( len > maxLength ) return false;
        }

        return intersected;
    }


}
