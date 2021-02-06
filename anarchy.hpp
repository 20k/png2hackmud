#ifndef ANARCHY_HPP_INCLUDED
#define ANARCHY_HPP_INCLUDED

#include <string>
#include <vector>

struct hackmud_char;

std::string radical_anarchy_charset();

std::string stenographic_encode(const std::string& in, const std::string& str);

std::string strip_colours(const std::string& in);

std::string stenographic_decode(const std::string& in);

size_t get_offset_of(const std::string& charset, char in);

std::string radical_shuffle(const std::string& in);

///takes a non coloured input image, creates a mapping string
std::string radical_anarchy(const std::string& in, const std::string& forcible_decode);

std::string radical_decode(const std::string& image, const std::string& mapping);
std::string radical_encode(const std::vector<hackmud_char>& image, const std::string& text, const std::string& radical_mapping);

#endif // ANARCHY_HPP_INCLUDED
