// SPDX-LICENSE-IDENTIFIER: GPL-3.0-or-later

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <popt.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

constexpr int border = 0;
constexpr int framewidth = 2;
constexpr int innerborder = 4;

#ifndef M_PI
constexpr double M_PI = 3.141592653589793238462643383279502884;
#endif

static cairo_t* background (int width, int height) {
    char svg[1024];

    cairo_surface_t* surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create (surface);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_paint (cr);
    RsvgRectangle viewport = { .x = 0.0, .y = 0.0, .width = width, .height = height };
    GError* error = NULL;
    snprintf (svg, sizeof(svg), "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
              "<svg width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\" xmlns:svg=\"http://www.w3.org/2000/svg\">"
              "<g id=\"layer1\">"
              "<rect style=\"opacity:1;fill:#000000;fill-opacity:0;stroke:#000000;stroke-width:%d;stroke-opacity:1\""
              " id=\"border\" width=\"%d\" height=\"%d\" x=\"%d\" y=\"%d\" ry=\"10\" rx=\"10\" />"
              "</g></svg>",
              width, height, width-1, height-1, framewidth,
              width-2*border-framewidth-1, height-2*border-framewidth-2,
              border+framewidth/2, border+framewidth/2);
    RsvgHandle* handle = rsvg_handle_new_from_data ((uint8_t*) svg, strlen(svg), &error);
    rsvg_handle_render_document (handle, cr, &viewport, &error);
    g_object_unref (handle);

    return cr;
}

static void add_text (cairo_t* cr, const char* face, int size, const char** lines) {
    cairo_surface_t* surface = cairo_get_group_target (cr);
    PangoLayout* layout = pango_cairo_create_layout (cr);
    PangoFontDescription* fontdesc = pango_font_description_new ();
    int width, height;

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
    if (face != NULL)
        pango_font_description_set_family_static (fontdesc, face);

    int linecount  = 0;
    for (const char** line=lines; *line; line++)
        linecount++;

    int maxwidth = cairo_image_surface_get_height (surface) - 2*(border+framewidth+innerborder);
    int maxheight = cairo_image_surface_get_width (surface) - 2*(border+framewidth+innerborder);

    if (size == 0) {
        if (linecount == 1)
            size = maxheight;
        else
            size = (maxheight * 6) / (linecount * 7);
    }
    pango_font_description_set_absolute_size (fontdesc, (double)size * PANGO_SCALE);
    pango_font_description_set_style (fontdesc, PANGO_STYLE_NORMAL);
    pango_layout_set_font_description (layout, fontdesc);
    pango_cairo_update_layout (cr, layout);
    pango_layout_get_size (layout, &width, &height);

    int extraspace = height / PANGO_SCALE - size;
    int textsize = linecount * size;
    int space = (maxheight - textsize) / (linecount + 1) - extraspace;
    int x = cairo_image_surface_get_width (surface) - (border+framewidth+innerborder) - space;
    for (const char** line=lines; *line; line++) {
        cairo_save (cr);
        pango_layout_set_text (layout, *line, -1);
        pango_cairo_update_layout (cr, layout);
        pango_layout_get_size (layout, &width, &height);
        if (width/PANGO_SCALE > maxwidth) {
            fprintf (stderr, "Text \"%s\" too long, aborting\n", *line);
            _exit (1);
        }
        int y = (cairo_image_surface_get_height (surface) - width/PANGO_SCALE) / 2;
        cairo_move_to (cr, x, y);
        cairo_rotate (cr, 90.0 * M_PI / 180.0);
        pango_cairo_update_layout (cr, layout);
        pango_cairo_show_layout (cr, layout);
        cairo_restore (cr);
        x -= space + height/PANGO_SCALE;
    }

    pango_font_description_free (fontdesc);
    g_object_unref (layout);
}

int main (int argc, const char * argv[]) {
    char * font = NULL;
    char * outfile = NULL;
    int width = 350, height = 106;
    int fontsize = 0;

    poptContext optcon;
    int c;
    const char ** text;

    const struct poptOption commandline_options[] = {
        { "font", 'f', POPT_ARG_STRING, &font, 0, "set font face to use", NULL },
        { "fontsize", 'p', POPT_ARG_INT, &fontsize, 0, "set font size in pixels", NULL },
        { "width", 'w', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &width, 0, "set width of label in pixels", NULL },
        { "height", 'h', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &height, 0, "set height of label in pixels", NULL },
        { "outfile", 'o', POPT_ARG_STRING, &outfile, 0, "output file name", NULL },
        POPT_AUTOHELP
        POPT_TABLEEND
    };

    optcon = poptGetContext (NULL, argc, argv, commandline_options, 0);
    while ((c = poptGetNextOpt (optcon)) >= 0);
    if (c < -1) {
        poptPrintHelp (optcon, stderr, 0);
        return 1;
    }

    text = poptGetArgs (optcon);
    if (text == NULL) {
        fprintf (stderr, "no data to print given\n");
        return 2;
    }
    if (outfile == NULL) {
        fprintf (stderr, "no output file name given\n");
        return 2;
    }

    cairo_t* cr = background (height, width);
    cairo_surface_t* surface = cairo_get_group_target (cr);

    add_text (cr, font, fontsize, text);

    if (cairo_surface_write_to_png (surface, outfile) != CAIRO_STATUS_SUCCESS) {
        fprintf (stderr, "could not write output file\n");
        return 1;
    }

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    return 0;
}
