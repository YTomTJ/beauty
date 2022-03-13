#pragma once

namespace beauty {
    namespace header
    {
        struct content_type {
            explicit content_type(const char* v) : value(v) {}
            std::string value;

            std::string operator()(const std::string&) const { return value; }
        };
    }
}

namespace beauty {
    namespace content_type {
        static const beauty::header::content_type text_plain         {"text/plain"};
        static const beauty::header::content_type text_html          {"text/html"};
        static const beauty::header::content_type application_json   {"application/json"};
        static const beauty::header::content_type image_x_icon       {"image/x-icon"};
        static const beauty::header::content_type image_png          {"image/png"};
    }
}
