#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
#include "lzw.hpp"
#include <set>
#include "anarchy.hpp"
#include "chars.hpp"

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

    c1 = srgb_to_lin(c1);

    c1 = c1 * 255.f;

    return c1.x() * 0.299 + c1.y() * 0.587 + c1.z() * 0.114;
}

float col2bright_lin(vec3f c1)
{
    return c1.x() * 0.299 + c1.y() * 0.587 + c1.z() * 0.114;
}

char col2ascii_full(vec3f c1, float brightness_scale)
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

char col2ascii_radical(vec3f c1, float brightness_scale)
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
char col2ascii_reduced(vec3f c1, float brightness_scale, vec3f min_val, vec3f max_val)
{
    vec3f lin_c1 = srgb_to_lin(c1);
    vec3f lin_min = srgb_to_lin(min_val);
    vec3f lin_max = srgb_to_lin(max_val);

    vec3f scaled_c1 = (lin_c1 - lin_min) / (lin_max - lin_min);

    ///scales from 0 to 1
    //c1 = (c1 - min_val) / (max_val - min_val);

    float bright = col2bright_lin(scaled_c1) * brightness_scale;

    bright = clamp(bright, 0.f, 1.f);

    bright = lin_to_srgb(bright);

    //std::string str = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\|()1{}[]?-_+~<>i!lI;:,^'. ";

    std::string str = " -+=*%#@";

    //std::string str = " ,-=+*%#@";

    int len = str.length();

    int id = floor(bright * (len - 1));

    id = clamp(id, 0, len-1);

    return str[id];
}

char col2ascii(vec3f c1, float brightness_scale, vec3f min_val, vec3f max_val)
{
    //return col2ascii_full(c1, brightness_scale);
    return col2ascii_reduced(c1, brightness_scale, min_val, max_val);
}

float get_col_err(vec3f c1, vec3f c2)
{
    vec3f linear_srgb_c1 = srgb_to_lin(c1/255.f);
    vec3f linear_srgb_c2 = srgb_to_lin(c2/255.f);

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

    float min_err = 50;

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

    vec3f min_val = {255, 255, 255};
    vec3f max_val = {0,0,0};

    for(int y=0; y<nimage.getSize().y; y++)
    {
        for(int x=0; x<nimage.getSize().x; x++)
        {
            sf::Color col = nimage.getPixel(x, y);

            min_val.x() = std::min(min_val.x(), (float)col.r);
            min_val.y() = std::min(min_val.y(), (float)col.g);
            min_val.z() = std::min(min_val.z(), (float)col.b);

            max_val.x() = std::max(max_val.x(), (float)col.r);
            max_val.y() = std::max(max_val.y(), (float)col.g);
            max_val.z() = std::max(max_val.z(), (float)col.b);
        }
    }

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
            hc.c = col2ascii({col.r, col.g, col.b}, brightness_scale, min_val, max_val);

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
