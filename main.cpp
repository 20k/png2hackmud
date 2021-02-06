#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
#include "lzw.hpp"
#include <set>

using namespace std;

float xyz_to_lab(float c)
{
    return c > 216.0 / 24389.0 ? pow(c, 1.0 / 3.0) : c * (841.0/108.0) + (4.0/29.0);
}

///0 -> 1
vec3f linear_rgb_to_lab(vec3f in)
{
    float R = in.x();
    float G = in.y();
    float B = in.z();

    float X, Y, Z;

    // linear sRGB -> normalized XYZ (X,Y,Z are all in 0...1)

    X = xyz_to_lab(R * (10135552.0/23359437.0) + G * (8788810.0/23359437.0) + B * (4435075.0/23359437.0));
    Y = xyz_to_lab(R * (871024.0/4096299.0)    + G * (8788810.0/12288897.0) + B * (887015.0/12288897.0));
    Z = xyz_to_lab(R * (158368.0/8920923.0)    + G * (8788810.0/80288307.0) + B * (70074185.0/80288307.0));

    // normalized XYZ -> Lab

    vec3f r;

    r.x() = Y * 116.0 - 16.0;
    r.y() = (X - Y) * 500.0;
    r.z() = (Y - Z) * 200.0;

    return r;
}

float col2bright(vec3f c1)
{
    c1 = c1 / 255.f;

    c1 = pow(c1, 1/2.2f);

    c1 = c1 * 255.f;

    return c1.x() * 0.299 + c1.y() * 0.587 + c1.z() * 0.114;
}

char col2ascii_full(vec3f c1, float brightness_scale = 1.f)
{
    float bright = col2bright(c1) * brightness_scale;

    bright /= 255.f;

    bright = clamp(bright, 0.f, 1.f);

    //bright = sqrt(bright);

    //std::string str = "@B%8&WM#*?_~<>;,^' ";
    std::string str = "$@B%8&WM#*oahkbdpqwmZO0QLJUYXzcvunxrjft|()1{}[]?_~<>ilI;,^' ";

    int len = str.length();

    int id = round(bright * (len - 1));

    id = clamp(id, 0, len-1);

    return str[str.length() - id - 1];
}

std::string radical_anarchy_charset()
{
    return "$@WM*#oahkbd%w=zcvu+x_~;,- ";
}

char col2ascii_radical(vec3f c1, float brightness_scale = 1.f)
{
    float bright = col2bright(c1) * brightness_scale;

    bright /= 255.f;

    bright = clamp(bright, 0.f, 1.f);

    //bright = sqrt(bright);
    std::string str = radical_anarchy_charset();

    int len = str.length();

    int id = round(bright * (len - 1));

    id = clamp(id, 0, len-1);

    return str[str.length() - id - 1];
}

///" .:-=+*#%@"

char col2ascii_reduced(vec3f c1, float brightness_scale = 1.f)
{
    float bright = col2bright(c1) * brightness_scale;

    bright /= 255.f;

    bright = clamp(bright, 0.f, 1.f);

    //std::string str = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\|()1{}[]?-_+~<>i!lI;:,^'. ";

    std::string str = " -+=*%#@";

    //std::string str = " ,-=+*%#@";

    int len = str.length();

    int id = round(bright * (len - 1));

    id = clamp(id, 0, len-1);

    return str[id];
}

char col2ascii(vec3f c1, float brightness_scale)
{
    //return col2ascii_full(c1, brightness_scale);
    return col2ascii_reduced(c1, brightness_scale);
}

float get_col_err(vec3f c1, vec3f c2)
{
    vec3f linear_srgb_c1 = pow(c1/255.f, 1.f/2.2f);
    vec3f linear_srgb_c2 = pow(c2/255.f, 1.f/2.2f);

    vec3f lab_c1 = linear_rgb_to_lab(linear_srgb_c1);
    vec3f lab_c2 = linear_rgb_to_lab(linear_srgb_c2);

    vec3f r1 = lab_c1 - lab_c2;

    return dot(r1, r1);

    vec3f rel = c1 - c2;

    return dot(rel, rel);
}

char get_nearest_col(sf::Color c, const std::map<char, vec3f>& colour_map)
{
    vec3f sf_c = {c.r, c.g, c.b};

    //sf_c = pow(sf_c, 2.2f);


    ///we should probably figure out how to make this gamma correct or w/e
    float min_err = 50;
    float min_err_sq = min_err * min_err;

    char fc = 'A';
    float found_err = FLT_MAX;

    for(auto& i : colour_map)
    {
        vec3f o = i.second;

        float err = get_col_err(sf_c, o);

        if(err < found_err)
        {
            fc = i.first;
            found_err = err;
        }
    }

    return fc;
}

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

std::map<char, vec3f> get_cmap()
{
    std::map<char, vec3f> colour_map;

    ///sigh
    colour_map['A'] = {240, 240, 240};
    colour_map['B'] = {234, 241, 241};
    colour_map['C'] = {206, 202, 203};
    colour_map['D'] = {243, 8, 8};
    colour_map['E'] = {242, 164, 160};
    colour_map['F'] = {243, 166, 3};
    colour_map['G'] = {243, 231, 130};
    colour_map['H'] = {243, 241, 7};
    colour_map['I'] = {243, 243, 113};
    colour_map['J'] = {243, 243, 7};
    colour_map['K'] = {242, 242, 193};
    colour_map['L'] = {42, 243, 8};
    colour_map['M'] = {231, 243, 204};
    colour_map['N'] = {5, 243, 243};
    colour_map['O'] = {179, 243, 243};
    colour_map['P'] = {8, 143, 242};
    colour_map['Q'] = {213, 242, 243};
    colour_map['R'] = {4, 8, 243};
    colour_map['S'] = {149, 228, 242};
    colour_map['T'] = {237, 60, 242};
    colour_map['U'] = {241, 236, 241};
    colour_map['V'] = {242, 6, 242};
    colour_map['W'] = {241, 186, 241};
    colour_map['X'] = {241, 6, 142};
    colour_map['Y'] = {240, 123, 195};
    colour_map['Z'] = {11, 16, 40};

    colour_map['a'] = {1,1,1};
    colour_map['b'] = {60,60,60};
    colour_map['c'] = {118,118,118};
    colour_map['d'] = {158,1,1};
    colour_map['e'] = {185,49,49};
    colour_map['f'] = {211,79,2};
    colour_map['g'] = {138,88,50};
    colour_map['h'] = {216,163,2};
    colour_map['i'] = {232,185,71};
    colour_map['j'] = {186,188,1};
    colour_map['k'] = {73,90,34};
    colour_map['l'] = {38,183,1};
    colour_map['m'] = {33,56,26};
    colour_map['n'] = {1,87,100};
    colour_map['o'] = {46,75,79};
    colour_map['p'] = {2,135,213};
    colour_map['q'] = {51,97,127};
    colour_map['r'] = {2,2,122};
    colour_map['s'] = {82,143,207};
    colour_map['t'] = {106,27,158};
    colour_map['u'] = {68,46,81};
    colour_map['v'] = {176,1,123};
    colour_map['w'] = {192,52,160};
    colour_map['x'] = {173,1,37};
    colour_map['y'] = {140,42,72};
    colour_map['z'] = {15,17,20};

    return colour_map;
}

std::vector<hackmud_char> get_full_image(const sf::Image& nimage, int max_w = 80, int max_h = 50)
{
    std::map<char, vec3f> colour_map = get_cmap();

    std::vector<hackmud_char> chars;

    for(int y=0; y<nimage.getSize().y; y++)
    {
        for(int x=0; x<nimage.getSize().x; x++)
        {
            sf::Color col = nimage.getPixel(x, y);

            char nearest_col = get_nearest_col(col, colour_map);

            vec3f real_col = {col.r, col.g, col.b};
            vec3f cur_col = colour_map[nearest_col];

            float brightness_scale = col2bright(real_col) / col2bright(cur_col);

            brightness_scale = clamp(brightness_scale, 0.f, 10.f);

            if(isnan(brightness_scale) || isinf(brightness_scale))
                brightness_scale = 1.f;

            hackmud_char hc;

            hc.is_newline = x == nimage.getSize().x - 1;
            hc.c = '@';
            hc.c = col2ascii({col.r, col.g, col.b}, brightness_scale);

            hc.colour = nearest_col;

            if(chars.size() != 0)
            {
                hc.try_merge(chars.back());
            }

            chars.push_back(hc);
        }
    }

    for(int i=1; i<chars.size(); i++)
    {
        chars[i].try_merge(chars[i-1]);
    }

    return chars;
}

std::vector<hackmud_char> limited_transition_bound(const std::string& img, int max_w, int max_h)
{
    std::vector<hackmud_char> chars_ret;

    sf::Image image;
    image.loadFromFile(img.c_str());


    sf::RenderTexture rtex;
    rtex.setSmooth(true);

    rtex.create(max_w, max_h);

    sf::Sprite spr;
    sf::Texture tex;
    tex.setSmooth(true);

    tex.loadFromImage(image);
    spr.setTexture(tex);

    spr.setScale((float)max_w/image.getSize().x, (float)max_h/image.getSize().y);

    rtex.draw(spr);
    rtex.display();

    sf::Image nimage;
    nimage = rtex.getTexture().copyToImage();

    chars_ret = get_full_image(nimage, max_w, max_h);

    nimage.saveToFile("TOUT.png");

    return chars_ret;
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

int main()
{
    std::map<char, vec3f> colour_map = get_cmap();

    //int max_w = 200/4;
    //int max_h = 100/4;

    std::vector<std::string> out;

    sf::Font font;

    font.loadFromFile("VeraMono.ttf");

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(10);
    txt.setString("A");

    //std::string fname = "mona_lisa.jpg";
    //std::string fname = "doggo.jpg";
    //std::string fname = "download.jpg";
    //std::string fname = "chirp_rip.png";
    //std::string fname = "nsfw/hackmud_nsfw.jpg";

    //std::string fname = "test_doge.png";

    std::string fname = "rick.jpg";

    ///stupid hack
    sf::Image img;
    img.loadFromFile(fname);

    int real_w = img.getSize().x;
    int real_h = img.getSize().y;

    int max_w = 15;
    int max_h = (real_h * max_w) / real_w;

    ///rick
    //max_w /= 6.5f;
    //max_h /= 8.5f;

    //max_w /= 3;
    //max_h /= 3;

    //max_w /= 1;
    //max_w = 90;
    //max_w /= 2;
    //max_h /= 1;

    ///setting for website
    //max_w /= 3.5;

    //max_w /= 5;
    //max_h /= 10;

    //max_w /= 65.5f;
    //max_h /= 105.5f;

    auto chars = limited_transition_bound(fname, max_w, max_h);

    //chars = stenographic_encode2(chars, "ifyou'rereadingthisthenyou'rebetteratthiskindofstuffthaniamimeanseriouslyhowareyouevendoingthis");

    /*for(auto& i : chars)
    {
        out.push_back(i.build());
    }

    std::string fully_built = "";

    for(auto& i : out)
    {
        fully_built += i;
    }

    std::string radical_mapping = radical_anarchy(fully_built, "cunt");

    std::cout << "RADICAL ANARCHY ACHIEVED " << radical_mapping << std::endl;

    std::cout << "radical decode " << radical_decode(fully_built, radical_mapping);


    std::string radical_encoded = radical_encode(chars, "urasmellypoobumbandihopeyou'remadeofcheese", radical_mapping);

    std::cout << "RADICAL REENCODE " << radical_encoded << std::endl;

    std::cout << "REDECODED " << radical_decode(radical_encoded, radical_mapping) << std::endl;

    //fully_built = stenographic_encode(fully_built, "POOP POOP POOP");

    //std::cout << "Decoded " << stenographic_decode(fully_built);

    //fully_built = stenographic_encode(fully_built, "poop");

    std::cout << std::endl;

    std::cout << fully_built;

    std::cout << std::endl;*/

    sf::RenderWindow win;
    win.create(sf::VideoMode(1600, 1000), "heheheh");

    std::cout << "\n\n\n\n\n\n";

    bool once = true;

    while(win.isOpen())
    {
        sf::Event event;

        while(win.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
            {
                win.close();
            }
        }

        for(int y=0; y<max_h; y++)
        {
            for(int x=0; x<max_w; x++)
            {
                hackmud_char hc = chars[y*max_w + x];

                std::string rs = std::string(1,hc.c);

                if(hc.is_newline)
                {
                    rs += "\n";
                }

                vec3f fc = colour_map[hc.colour];
                sf::Color sc = sf::Color(fc.x(), fc.y(), fc.z());

                if(once)
                std::cout << rs;

                txt.setPosition(x*10, y*10);
                txt.setString(rs);
                txt.setColor(sc);

                win.draw(txt);
            }

            //if(once)
            //std::cout << std::endl;
        }

        once = false;

        win.display();
        win.clear();
    }

    //std::cout << out << std::endl;

    return 0;
}
