#include "anarchy.hpp"
#include <random>
#include <algorithm>
#include <set>
#include <iostream>
#include <map>
#include <assert.h>
#include "chars.hpp"

std::string radical_anarchy_charset()
{
    return "$@WM*#oahkbd%w=zcvu+x_~;,- ";
}

std::string stenographic_encode(const std::string& in, const std::string& str)
{
    std::string ret;

    int msg_bit = 0;

    bool is_col = false;
    bool skip = false;
    bool has_col = false;

    for(int i=0; i < (int)in.size(); i++)
    {
        char c = in[i];

        skip = false;

        if(is_col)
        {
            is_col = false;
            skip = true;
        }

        if(c == '`' && has_col && !skip)
        {
            is_col = false;
            skip = true;
            has_col = false;
        }

        if(c == '`' && !has_col && !skip)
        {
            is_col = true;
            skip = true;
            has_col = true;
        }

        if(c == '\n')
        {
            skip = true;
        }

        if(!skip)
        {
            int msg_idx = msg_bit / 8;

            if(msg_idx < (int)str.size())
            {
                int bit = msg_bit % 8;

                uint8_t in_char = str[msg_idx];

                int ins = (in_char >> bit) & 1;

                if(ins)
                {
                    c = '@';
                }
                else
                {
                    c = '#';
                }
            }

            msg_bit++;
        }

        ret.push_back(c);
    }

    return ret;
}

std::string strip_colours(const std::string& in)
{
    std::string ret;

    bool is_col = false;
    bool has_col = false;

    for(char c : in)
    {
        if(c == '`' && !has_col)
        {
            is_col = true;
            has_col = true;
            continue;
        }

        if(c == '`' && has_col)
        {
            has_col = false;
            continue;
        }

        if(is_col)
        {
            is_col = false;
            continue;
        }

        //if(c == '\n')
        //    continue;

        ret += std::string(1, c);
    }

    return ret;
}

std::string stenographic_decode(const std::string& in)
{
    std::string uncol = strip_colours(in);

    std::string ret;

    uint8_t cur = 0;

    int counter = 0;

    bool is_col = false;
    bool has_col = false;

    for(char c : uncol)
    {
        if(c == '\n')
            continue;

        int cbit = counter % 8;

        if(cbit == 0 && counter != 0)
        {
            ret += std::string(1, cur);
            cur = 0;
        }

        int bit = 0;

        if(c == '#' || c == '@')
        {
            if(c == '@')
                bit = 1;

            if(bit)
            {
                cur |= (1 << cbit);
            }
        }
        else
        {
            return ret;
        }

        counter++;
    }

    ret += std::string(1, cur);

    return ret;
}

size_t get_offset_of(const std::string& charset, char in)
{
    for(int i=0; i < (int)charset.size(); i++)
    {
        if(charset[i] == in)
            return i;
    }

    return -1;
}

struct radical_character
{
    char one = 0;
    char two = 0;
};

std::string radical_shuffle(const std::string& in)
{
    std::vector<radical_character> tin;

    if((in.size() % 2) != 0)
        return "";

    for(int i=0; i < (int)in.size(); i+=2)
    {
        tin.push_back({in[i], in[i+1]});
    }

    std::random_device rd;
    std::mt19937 g(1);

    std::shuffle(tin.begin(), tin.end(), g);

    std::string out;

    for(auto& i : tin)
    {
        out += std::string(1, i.one) + std::string(1, i.two);
    }

    return out;
}

///takes a non coloured input image, creates a mapping string
std::string radical_anarchy(const std::string& in, const std::string& forcible_decode)
{
    size_t constant = 0;
    size_t modulo = 1;
    size_t text_offset = 0;

    std::string without_colours = strip_colours(in);

    int to_check = forcible_decode.size();
    int check_success = 0;

    bool found = false;

    std::set<char> foundchars;

    for(int i=1; i < without_colours.size(); i++)
    {
        char test_char = without_colours[i];

        if(foundchars.find(test_char) == foundchars.end())
        {
            foundchars.insert(test_char);
        }
        else
        {
            foundchars.clear();
        }

        if(test_char == '\n')
            foundchars.clear();

        if(foundchars.size() == to_check)
        {
            found = true;
            text_offset = i - (to_check - 1);
            break;
        }
    }

    if(found)
    {
        for(int i=text_offset; i < text_offset + forcible_decode.size(); i++)
        {
            std::cout << "f1 " << std::string(1, without_colours[i]) << std::endl;
        }
    }

    if(!found)
    {
        printf("Did not find radical anarchy");
    }
    else
    {
        std::string found_chars = "";

        for(int i=0; i < to_check; i++)
        {
            found_chars += without_colours[text_offset + i];
        }

        std::cout << "found radical anarchy at " << text_offset << std::endl;

        std::string radical_charset = radical_anarchy_charset();

        assert(radical_charset.size() >= to_check);

        ///for the full characterset
        ///maps a lowercase a-z to the radical characterset
        //std::vector<int> diffs;
        std::map<char, char> radical_map;
        std::map<char, bool> used_alphabet;
        std::map<char, bool> used_radical;


        for(int i=0; i < to_check; i++)
        {
            char radical_char = without_colours[i + text_offset];
            char real_char = forcible_decode[i];

            radical_map[real_char] = radical_char;
            used_alphabet[real_char] = true;
            used_radical[radical_char] = true;
        }

        for(int i=0; i < 26; i++)
        {
            char real_char = 'a' + i;

            if(used_alphabet[real_char])
                continue;

            int first_valid = -1;

            for(int kk=0; kk < 26; kk++)
            {
                char radical_test = radical_anarchy_charset()[kk];

                if(used_radical[radical_test])
                    continue;

                first_valid = kk;
            }

            if(first_valid == -1)
                assert(false);

            radical_map[real_char] = radical_anarchy_charset()[first_valid];
            used_radical[radical_anarchy_charset()[first_valid]] = true;
            used_alphabet[real_char] = true;
        }

        std::string mapping_string;

        for(auto& i : radical_map)
        {
            //std::cout << "mapping " << i.first << " to " << i.second << std::endl;

            mapping_string += std::string(1, i.first) + std::string(1, i.second);
        }

        int polynomial_offset = 0;
        std::string visual_polynomial;

        std::vector<size_t> accumulator;

        for(int i=0; i < mapping_string.size(); i += 8)
        {
            size_t current = 0;

            for(int k=0; k < sizeof(current) && k + i < mapping_string.size(); k++)
            {
                size_t value = mapping_string[i + k];

                current += (value << (size_t)(k*8));
            }

            std::cout << "ival " << i << std::endl;

            accumulator.push_back(current);
        }

        std::cout << "64bit " << accumulator.size() << std::endl;

        /*for(auto& i : mapping_string)
        {
            int offset = polynomial_offset * 8;

            visual_polynomial += "(" + (std::to_string((int)i) + std::string("<<") + std::to_string(offset)) + ") + ";

            polynomial_offset++;
        }*/

        for(auto& i : accumulator)
        {
            int offset = polynomial_offset * 64;

            visual_polynomial += "(" + (std::to_string((size_t)i) + std::string("<<") + std::to_string(offset)) + ") + ";

            polynomial_offset++;
        }

        std::cout << visual_polynomial << std::endl;

        //std::cout << "lzw size? " << lzw_encode(mapping_string).size() << std::endl;

        assert(radical_map.size() == 26);

        return radical_shuffle(mapping_string);
    }

    return "";
}

std::string radical_decode(const std::string& image, const std::string& mapping)
{
    std::map<char, char> radical;

    if((mapping.size() % 2) != 0)
        return "This is not a radical package";

    for(int i=0; i < (int)mapping.size(); i += 2)
    {
        radical[mapping[i+1]] = mapping[i];
    }

    std::string stripped = strip_colours(image);

    std::string str;

    for(auto& i : stripped)
    {
        if(i == '\n')
        {
            str += i;
            continue;
        }

        if(radical.find(i) == radical.end())
        {
            str += 'X';
            continue;
        }

        str += radical[i];
    }

    return str;
}

std::string radical_encode(const std::vector<hackmud_char>& image, const std::string& text, const std::string& radical_mapping)
{
    std::string ret;

    std::vector<hackmud_char> mimage = image;

    int idx = 0;

    std::map<char, char> radical_map;

    for(int i=0; i < (int)radical_mapping.size(); i+=2)
    {
        radical_map[radical_mapping[i]] = radical_mapping[i+1];
    }

    for(; idx < text.size() && idx < mimage.size(); idx++)
    {
        mimage[idx].c = radical_map[text[idx]];
    }

    for(auto& i : mimage)
    {
        ret += i.build();
    }

    return ret;
}
