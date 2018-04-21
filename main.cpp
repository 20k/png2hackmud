#include <iostream>
#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <map>
#include <cstdlib>
#include <algorithm>

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

    std::string str = "$@B%8&WM#*oahkbdpqwmZO0QLJUYXzcvunxrjft|()1{}[]?_~<>ilI;,^' ";

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


char get_nearest_c(vec3f c, const std::map<char, vec3f>& colour_map)
{
    return get_nearest_col(sf::Color(c.x(), c.y(), c.z()), colour_map);
}

///so its graygraygray red red green green blue blue
char get_nearest_col_q(sf::Color c, const std::map<char, vec3f>& colour_map)
{
    int w = 0;
    float h = c.r;

    if(c.g > h)
    {
        w = 1;
        h = c.g;
    }

    if(c.b > h)
    {
        w = 2;
        h = c.b;
    }

    std::vector<char> cols;

    cols.push_back(get_nearest_c({0,0,0}, colour_map));
    cols.push_back(get_nearest_c({128, 128, 128}, colour_map));
    cols.push_back(get_nearest_c({255, 255, 255}, colour_map));


    cols.push_back(get_nearest_c({128, 0, 0}, colour_map));
    cols.push_back(get_nearest_c({255, 0, 0}, colour_map));

    cols.push_back(get_nearest_c({0, 128, 0}, colour_map));
    cols.push_back(get_nearest_c({0, 255, 0}, colour_map));

    cols.push_back(get_nearest_c({0, 0, 128}, colour_map));
    cols.push_back(get_nearest_c({0, 0, 255}, colour_map));

    float brightness = 0.299*c.r + 0.587*c.g + 0.114*c.b;
    int blevel = 0;

    if(brightness > (128 + 255)/2.f)
    {
        blevel = 1;
    }

    if(brightness < 128/2.f)
    {
        return cols[0];
    }


    return cols[w*2 + blevel + 3];
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
            ret = ret + SPACE + "\\n";
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

    void eliminate_single(hackmud_char& prev, hackmud_char& forw)
    {
        bool double_equal = false;

        if(!prev.is_newline && !forw.is_newline)
        {
            if(prev.colour != colour && forw.colour != colour)
                double_equal = true;
        }

        if(prev.is_newline && !forw.is_newline)
        {
            if(colour != forw.colour)
                double_equal = true;
        }

        if(!prev.is_newline && forw.is_newline)
        {
            if(colour != prev.colour)
                double_equal = true;
        }


        //if(prev.colour != colour && forw.colour != colour)
        if(double_equal)
        {
            if(!prev.is_newline)
                colour = prev.colour;
            else
                colour = forw.colour;
        }
    }
};


std::map<char, vec3f> quantise_colour_map(std::map<char, vec3f>& cmap)
{
    int grays = 3;

    std::vector<char> cols;

    cols.push_back(get_nearest_c({0,0,0}, cmap));
    cols.push_back(get_nearest_c({128, 128, 128}, cmap));
    cols.push_back(get_nearest_c({255, 255, 255}, cmap));


    cols.push_back(get_nearest_c({128, 0, 0}, cmap));
    cols.push_back(get_nearest_c({255, 0, 0}, cmap));

    cols.push_back(get_nearest_c({0, 128, 0}, cmap));
    cols.push_back(get_nearest_c({0, 255, 0}, cmap));

    cols.push_back(get_nearest_c({0, 0, 128}, cmap));
    cols.push_back(get_nearest_c({0, 0, 255}, cmap));

    std::map<char, vec3f> cm;

    for(auto& i : cols)
    {
        cm[i] = cmap[i];
    }

    return cm;
}

int get_num_transitions(const std::vector<std::string>& out)
{
    int c = 0;

    for(auto& i : out)
    {
        size_t n = std::count(i.begin(), i.end(), '`');

        c += n;
    }

    if(c % 2 != 0)
    {
        printf("warning, mismatched `\n");
    }

    return c / 2;
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

std::vector<hackmud_char> get_full_image(const sf::Image& nimage, int max_w = 80, int max_h = 50, float frac = 0.f)
{
    std::map<char, vec3f> colour_map = get_cmap();

    int num_to_erase = 0;

    //float erase_prob = (float)num_to_erase / colour_map.size();

    float erase_prob = frac;

    erase_prob = 0.f;

    for(auto it = colour_map.begin(); it != colour_map.end(); it++)
    {
        if(randf_s(0.f, 1.f) < erase_prob)
        {
            colour_map.erase(it);
            it--;
        }
    }

    //std::map<char, vec3f> quantised_colour_map = quantise_colour_map(colour_map);

    //colour_map = quantised_colour_map;

    /*for(auto& i : colour_map)
    {
        char tl = tolower(i.first);

        if(tl == i.first)
            continue;

        colour_map[tl] = i.second/2.f;
    }*/

    //nimage.saveToFile("Fname.png");

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

    ///appears to be fixed in live
    #ifdef FIX_ONECHARACTER_BUG
    for(int i=1; i<chars.size()-1; i++)
    {
        if(SPACE == "")
            chars[i].eliminate_single(chars[i-1], chars[i+1]);
    }
    #endif

    for(int i=1; i<chars.size(); i++)
    {
        chars[i].try_merge(chars[i-1]);
    }

    return chars;
}

std::vector<hackmud_char> limited_transition_bound(const std::string& img, int max_w, int max_h, int transition_limit = 400)
{
    int num_transitions = -1;

    int max_attemps = 50;
    int attempts = 0;

    std::vector<std::string> out;
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

    nimage.saveToFile("TOUT.png");

    int search_depth = 5;

    float valid_val = 1.f;
    float invalid_val = 0.f;

    //while((num_transitions == -1 || num_transitions >= transition_limit))
    for(int i=0; i<search_depth; i++)
    {
        //float elim = (float)attempts / max_attemps;

        float elim = (valid_val + invalid_val) / 2.f;

        chars_ret = get_full_image(nimage, max_w, max_h, elim);

        out.clear();

        for(auto& i : chars_ret)
        {
            out.push_back(i.build());
        }

        num_transitions = get_num_transitions(out);

        printf("na %f %i\n", elim, num_transitions);

        if(num_transitions < transition_limit)
        {
            valid_val = elim;
        }
        if(num_transitions >= transition_limit)
        {
            invalid_val = elim;
        }

        if(num_transitions > transition_limit - 30 && num_transitions < transition_limit)
        {
            return chars_ret;
        }
    }

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

    std::string fname = "mona_lisa.jpg";
    //std::string fname = "rick2.jpg";
    ///stupid hack
    sf::Image img;
    img.loadFromFile(fname);

    int max_w = img.getSize().x;
    int max_h = img.getSize().y;

    max_w /= 4.5f;
    max_h /= 6.5f;

    auto chars = limited_transition_bound(fname, max_w, max_h, 300);

    for(auto& i : chars)
    {
        out.push_back(i.build());
    }

    std::cout << std::endl;

    for(auto& i : out)
    {
        std::cout << i;
    }

    std::cout << std::endl;

    int num_transitions = get_num_transitions(out);

    std::cout << "num transitions " << num_transitions << std::endl;

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
