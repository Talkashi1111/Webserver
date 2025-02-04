// File: utils.hpp
//
// This file contains utility functions that are used throughout the project.

#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>

// template function to convert any type to a std::string
template <typename T>
static std::string toString (const T &t)
{
    std::ostringstream oss;
    oss << t;
    return (oss.str());
}

#endif // UTILS_HPP