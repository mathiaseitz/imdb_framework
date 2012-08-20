/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef PROGRESS_HPP
#define PROGRESS_HPP

#include <iostream>

/**
 * @ingroup io
 * @brief Helper class encapsulating some common progress output functionality for
 * our command-line tools
 */
struct progress_output
{
    progress_output(int interval = 1000) : _interval(interval) {}

    /**
     * @brief Progress output operator in percent, requires that you know the total number of elements to be processed.
     *
     * @param current Current element index in [0,total)
     * @param total Total number of elements to be processed.
     * @param prefix Prefix to be displayed before the actual percent output
     */
    inline void operator() (int current, int total, const std::string& prefix) const
    {
        static const char whirl[4] = { '-', '\\', '|', '/' };

        ++current;
        bool last = (current == total);

        if (current % _interval == 0 || last)
        {
            std::cout << prefix << whirl[(current / _interval) % 4] << " "
                      << current << " of " << total << " (" << int(current * 100.0 / total) << "%)"
                      << '\r' << std::flush;
        }

        if (last)
        {
            std::cout << std::endl;
        }
    }


    /**
     * @brief Progress output operator displaying the total number of elements already processed
     *
     * Use this if you do not know the total number of elements to be processed beforehand, e.g.
     * when listing all files in a directory
     * @param current Current element
     * @param prefix Prefix to be displayed before the actual progress output
     */
    inline void operator() (int current, const std::string& prefix) const
    {
        static const char whirl[4] = { '-', '\\', '|', '/' };

        if (current % _interval == 0)
        {
            std::cout << prefix << whirl[(current / _interval) % 4] << " "
                      << current << '\r' << std::flush;
        }
    }

    private:

    int _interval;
};

#endif // PROGRESS_HPP
