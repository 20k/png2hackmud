#ifndef CHARS_HPP_INCLUDED
#define CHARS_HPP_INCLUDED

#include <string>

#define SPACE ""

struct hackmud_char
{
    char c = '@';
    char colour = 'A';
    bool is_newline = false;
    bool is_merged = false;
    bool forward_merge = false;
    bool backward_merge = false;

    std::string build()
    {
        //std::string ret = std::string("`") + std::string(1, colour) + std::string(1, c) + " `";

        std::string ret;

        if(!backward_merge && !forward_merge)
        {
            ret = std::string("`") + std::string(1, colour) + std::string(1, c) + SPACE + "`";
        }

        if(!backward_merge && forward_merge)
        {
            ret = std::string("`") + std::string(1, colour) + std::string(1, c) + SPACE;
        }

        if(backward_merge && !forward_merge)
        {
            ret = std::string(1, c) + SPACE + "`";
            //ret = std::string(1, 'B') + " `";
        }

        if(forward_merge && backward_merge)
        {
            ret = std::string(1, c) + SPACE;
            //ret = std::string(1, 'M') + " ";
        }


        if(is_merged)
        {
            //ret = std::string(1, c) + " ";
        }

        if(is_newline)
        {
            ret = ret + SPACE + "\n";
            //ret = ret + SPACE + "\\n";
        }

        return ret;
    }

    void try_merge(hackmud_char& prev)
    {
        if(prev.colour == colour && !prev.is_newline)
        {
            is_merged = true;
            prev.forward_merge = true;
            backward_merge = true;

            //printf("%c %c ", prev.colour, colour);
        }
    }
};

#endif // CHARS_HPP_INCLUDED
