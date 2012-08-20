/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef CMDLINE_HPP
#define CMDLINE_HPP

#include <string>
#include <vector>
#include <iostream>

#include <boost/lexical_cast.hpp>

namespace imdb
{

/**
 * @ingroup io
 * @brief Defines the structure of a short commandline option.
 *
 * In this implementation a short command line option starts with a
 * '-' and is then followed by exactly one alphabetic character, e.g. -t
 */
inline bool is_short_option(const std::string& s)
{
    return (s.length() == 2 && s[0] == '-' && std::isalpha(s[1]));
}

/**
 * @ingroup io
 * @brief Defines the structure of a long commandline option.
 *
 * In this implementation a long command line option starts with '--'
 * and is then followed by at least one alphabetic character, followed
 * by arbitrary characters, e.g. --parameters
 */
inline bool is_long_option(const std::string& s)
{
    return (s.length() > 2 && s[0] == '-' && s[1] == '-' && std::isalpha(s[2]));
}

std::vector<std::string> argv_to_strings(int argc, char* argv[])
{
    std::vector<std::string> strings;
    for (int i = 0; i < argc; i++) strings.push_back(argv[i]);
    return strings;
}

/**
 * @ingroup io
 * @brief Encodes a single commandline option including a brief description
 *
 * A CmdOption encodes a single commandline option that a user can provide to a command as well as the parameters that
 * are supported by this option.
 *
 * Here is an example: assume we have a command that supports a single option
 * '--sigma' which has a single floating point parameter, i.e. a use would use --sigma 0.5
 */
class CmdOption
{
    public:


    /**
     * @brief Constructs a command option, defined by a long and short option and a description
     * @param long_option Single word that describes the command option reasonably, e.g. sigma
     * @param short_option Typically a single letter, e.g. s
     * @param description One ore more sentences describing the command
     */
    CmdOption(const std::string& long_option, const std::string& short_option, const std::string& description)
        : _long_option(long_option)
        , _short_option(short_option)
        , _description(description)
    {}


    /**
     * @brief Parse a single parameter from the commandline option.
     *
     * If the user provided --sigma 0.5 you would use T=float and would get back the value 0.5
     */
    template <class T>
    bool parse_single(const std::vector<std::string>& args, T& value)
    {
        bool found = false;

        for (int i = 0; i < (int)args.size() - 1; i++)
        {
            if (match(args[i]))
            {
                if (is_short_option(args[i + 1]) || is_long_option(args[i + 1])) continue;

                try
                {
                    value = boost::lexical_cast<T>(args[i + 1]);
                    found = true;
                }
                catch (boost::bad_lexical_cast&)
                {
                    std::cerr << "bad parameter value: " << args[i + 1] << std::endl;
                }
            }
        }

        return found;
    }

    /**
     * @brief Parse multiple parameters from a commandline option.
     *
     * If the user provided --input file1.txt file2.txt you would use T=std::string and would get back a
     * vector<std::string> containing "file1.txt" and "file2.txt"
     */
    template <class T>
    bool parse_multiple(const std::vector<std::string>& args, std::vector<T>& values)
    {
        bool found = false;

        for (int i = 0; i < (int)args.size() - 1; i++)
        {
            if (match(args[i]))
            {
                for (size_t k = i+1; k < args.size(); k++ )
                {
                    if (is_short_option(args[k]) || is_long_option(args[k])) break;

                    try
                    {
                        values.push_back(boost::lexical_cast<T>(args[k]));
                        found = true;
                    }
                    catch (boost::bad_lexical_cast&)
                    {
                        std::cerr << "bad parameter value: " << args[k] << std::endl;
                    }
                }
            }
        }

        return found;
    }

    bool match(const std::string& arg)
    {
        return ((is_short_option(arg) && arg.compare(1, arg.length()-1, _short_option) == 0) ||
                (is_long_option(arg) && arg.compare(2, arg.length()-2, _long_option) == 0));
    }

    const std::string& long_option() const { return _long_option; }
    const std::string& short_option() const { return _short_option; }
    const std::string& description() const { return _description; }

    private:

    std::string _long_option;
    std::string _short_option;
    std::string _description;
};


/**
 * @ingroup io
 * @brief Base class for a commandline options parser.
 *
 * Derive from this class and add the desired CmdOption instances to define the behaviour of your
 * specific commandline parser. Derived classes need to implement run(). Inside run(), you extract
 * the commandline parameters as passed in by the user and pass those to your actual program.
 */
class Command
{
    public:

    Command(const std::string& usage = "") : _usage(usage)
    {}

    /**
     * @brief Add as many CmdOption instances as desired.
     */
    void add(const CmdOption& option)
    {
        _options.push_back(option);
    }

    /**
     * @brief Check for undefined commandline options passed in by the user.
     *
     * Undefined commandline options are those that are not supported by any of the CmdOption
     * instances add to this Command.
     */
    std::vector<std::string> check_for_unknown_option(const std::vector<std::string>& args)
    {
        std::vector<std::string> unknown;

        for (size_t i = 0; i < args.size(); i++)
        {
            if (!is_short_option(args[i]) && !is_long_option(args[i])) continue;

            bool found = false;

            for (size_t k = 0; k < _options.size() && !found; k++)
            {
                if (_options[k].match(args[i])) found = true;
            }

            if (!found) unknown.push_back(args[i]);
        }

        return unknown;
    }

    void warn_for_unknown_option(const std::vector<std::string>& args)
    {
        std::vector<std::string> unknown = check_for_unknown_option(args);
        for (size_t i = 0; i < unknown.size(); i++)
        {
            std::cerr << "WARNING: unknown option: " << unknown[i] << std::endl;
        }
    }

    void print() const
    {
        static const int c0 = 30;

        std::cout << _usage << std::endl;

        if (_options.size() > 0) std::cout << "options:" << std::endl;

        for (size_t i = 0; i < _options.size(); i++)
        {
            std::string line = "  --" + _options[i].long_option() + ", -" + _options[i].short_option();
            for (int k = line.length(); k < c0; k++) line += ' ';
            std::cout << line << _options[i].description() << std::endl;
        }
    }

    virtual bool run(const std::vector<std::string>& /*args*/)
    {
        return false;
    }

    private:

    std::vector<CmdOption> _options;
    std::string            _usage;
};

} // namespace imdb

#endif // CMDLINE_HPP
