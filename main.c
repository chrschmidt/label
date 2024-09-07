// SPDX-LICENSE-IDENTIFIER: GPL-3.0-or-later

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <popt.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>

// M_PI is a difference between POSIX C and the C standard itself
#ifndef M_PI
constexpr double M_PI = 3.141592653589793238462643383279502884;
#endif

struct {
// Canvas
    int width;
    int height;
    int rotate;
// Border / frame
    int drawframe;
    int outerborder;
    int innerborder;
    int rightborder;
    int bottomborder;
    int leftborder;
    int topborder;
    int stroke;
    int roundness;
} config = {
    .width        = 350,
    .height       = 106,
    .rotate       = 0,
    .drawframe    = 1,
    .outerborder  = 0,
    .innerborder  = 4,
    .rightborder  = 0,
    .bottomborder = 0,
    .leftborder   = 0,
    .topborder    = 0,
    .stroke       = 2,
    .roundness    = 10,
};

static cairo_t* background (void) {
    cairo_surface_t* surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, config.width, config.height);
    cairo_t* cr = cairo_create (surface);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_paint (cr);
    if (config.drawframe) {
        config.rightborder  += config.outerborder;
        config.bottomborder += config.outerborder;
        config.leftborder   += config.outerborder;
        config.topborder    += config.outerborder;
        int drawwidth  = config.width  - config.leftborder - config.rightborder;
        int drawheight = config.height - config.topborder  - config.bottomborder;
        int stroke = config.stroke;
        char svg[1024];
        RsvgRectangle viewport = { .x = config.leftborder, .y = config.topborder, .width = drawwidth, .height = drawheight };
        GError* error = NULL;
        snprintf (svg, sizeof(svg), "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
                  "<svg width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\" xmlns:svg=\"http://www.w3.org/2000/svg\">"
                  "<g id=\"layer1\">"
                  "<rect style=\"opacity:1;fill:white;fill-opacity:0;stroke:black;stroke-width:%d;stroke-opacity:1\""
                  " id=\"border\" width=\"%d\" height=\"%d\" x=\"%d\" y=\"%d\" ry=\"10\" rx=\"10\" />"
                  "</g></svg>",
                  drawwidth, drawheight, drawwidth-1, drawheight-1,          // svg size and viewport
                  stroke,                                                    // stroke width
                  (int)(drawwidth-1.5*stroke), (int)(drawheight-1.5*stroke), // width and height, in the centerline of the object
                  stroke/2, stroke/2);
        RsvgHandle* handle = rsvg_handle_new_from_data ((uint8_t*) svg, strlen(svg), &error);
        rsvg_handle_render_document (handle, cr, &viewport, &error);
        g_object_unref (handle);
    }
    return cr;
}

static void add_text (cairo_t* cr, const char* face, int size, const char** lines) {
    PangoLayout* layout = pango_cairo_create_layout (cr);
    PangoFontDescription* fontdesc = pango_font_description_new ();
    int width, height;

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
    if (face != NULL)
        pango_font_description_set_family_static (fontdesc, face);

    int linecount  = 0;
    for (const char** line=lines; *line; line++)
        linecount++;

    int drawwidth = config.width - config.rightborder - config.leftborder - 2*config.innerborder - 2*config.stroke;
    int drawheight = config.height - config.topborder - config.bottomborder - 2*config.innerborder - 2*config.stroke;
    int ystart = config.topborder  + config.innerborder + config.stroke;
    int xstart = config.leftborder + config.innerborder + config.stroke;

    int space = drawheight / linecount;
    int y;
    if (size == 0) {
        size = space * 7 / 8;
        y = ystart;
    } else {
        y = ystart; // TBD
    }
    printf ("Using line spacing %d, font size: %d pixel\n", space, size);
    pango_font_description_set_absolute_size (fontdesc, (double)size * PANGO_SCALE);
    pango_font_description_set_style (fontdesc, PANGO_STYLE_NORMAL);
    pango_layout_set_font_description (layout, fontdesc);
    pango_cairo_update_layout (cr, layout);

    for (const char** line=lines; *line; line++) {
        printf ("Writing> %s", *line);
        cairo_save (cr);
        pango_layout_set_text (layout, *line, -1);
        pango_cairo_update_layout (cr, layout);
        pango_layout_get_pixel_size (layout, &width, &height);
//        PangoRectangle inkr, logr;
//        pango_layout_get_pixel_extents (layout, &inkr, &logr);

        if (width > drawwidth) {
            fprintf (stderr, "Text \"%s\" too long, aborting\n", *line);
            _exit (1);
        }
        printf ("  size: %dx%d\n", width, height);
        int x = xstart + drawwidth/2 - (width/2);
        cairo_move_to (cr, x, y);
        pango_cairo_update_layout (cr, layout);
        pango_cairo_show_layout (cr, layout);
        cairo_restore (cr);
        y += space;
    }
    pango_font_description_free (fontdesc);
    g_object_unref (layout);
}

static cairo_t* rotate (cairo_t* cr) {
    int width;
    int height;
    int xshift = 0;
    int yshift = 0;
    switch (config.rotate) {
    case 0:
    case 180:
        width  = config.width;
        height = config.height;
        break;
    default:
        width  = config.height;
        height = config.width;
    }
    switch (config.rotate) {
    case 90:
        yshift = -width;
        break;
    case 180:
        xshift = -width;
        yshift = -height;
        break;
    case 270:
        xshift = -height;
        break;
    }

    cairo_surface_t* surface  = cairo_get_group_target (cr);
    cairo_surface_t* surface2 = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr2 = cairo_create (surface2);

    if (config.rotate != 0) {
        cairo_rotate (cr2, (double)config.rotate * M_PI / 180.0);
        cairo_translate (cr2, xshift, yshift);
    }
    cairo_set_source_surface (cr2, surface, 0, 0);
    cairo_set_operator (cr2, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr2);
    return cr2;
}

int main (int argc, const char * argv[]) {
    char * font = NULL;
    char * outfile = NULL;
    int fontsize = 0;

    poptContext optcon;
    int c;
    const char ** text;

    const struct poptOption commandline_options[] = {
        { "font", 'f', POPT_ARG_STRING, &font, 0, "set font face to use", NULL },
        { "fontsize", 'p', POPT_ARG_INT, &fontsize, 0, "set font size in pixels", NULL },
        { "width", 'w', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &config.width, 0, "set width of label in pixels", NULL },
        { "height", 'h', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &config.height, 0, "set height of label in pixels", NULL },
        { "rotate", 'r', POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT, &config.rotate, 0, "rotation of the image", NULL },
        { "noframe", 0, POPT_ARG_VAL, &config.drawframe, 0, "do not draw a box around the text", NULL },
        { "ob", 0, POPT_ARG_INT, &config.outerborder, 0, "extra border space around the frame", NULL },
        { "ib", 0, POPT_ARG_INT, &config.innerborder, 0, "extra border space inside the frame", NULL },
        { "rb", 0, POPT_ARG_INT, &config.rightborder, 0, "extra outer border space to the right", NULL },
        { "lb", 0, POPT_ARG_INT, &config.leftborder, 0, "extra outer border space to the left", NULL },
        { "tb", 0, POPT_ARG_INT, &config.topborder, 0, "extra outer border space to the top", NULL },
        { "bb", 0, POPT_ARG_INT, &config.bottomborder, 0, "extra outer border space to the bottom", NULL },
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

    switch (config.rotate) {
    case 0:
    case 90:
    case 180:
    case 270:
        break;
    default:
        fprintf (stderr, "invalid rotation %d°. Allowed values are 0, 90, 180 or 270\n", config.rotate);
        return 2;
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

    cairo_t* cr = background ();
    add_text (cr, font, fontsize, text);

    cairo_t* cr2 = rotate (cr);
    cairo_surface_t* surface = cairo_get_group_target (cr2);
    if (cairo_surface_write_to_png (surface, outfile) != CAIRO_STATUS_SUCCESS) {
        fprintf (stderr, "could not write output file\n");
        return 1;
    }

    cairo_destroy (cr2);
    cairo_destroy (cr);
    return 0;
}
