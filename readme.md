## This program

This is a small tool to generate png graphics for use with a label printer
from command line.

## Usage

    ./label -h
    Usage: label [OPTION...]
      -f, --font=STRING        set font face to use
      -p, --fontsize=INT       set font size in pixels
      -w, --width=INT          set width of label in pixels (default: 350)
      -h, --height=INT         set height of label in pixels (default: 106)
      -r, --rotate=INT         rotation of the image (default: 0)
      -b, --noborder           do not draw a box around the text
      -o, --outfile=STRING     output file name
      -h, --help               this help screen

The default size is set for 12mm paper of a Brother QL label printer (106 pixel height).

Each further argument is interpreted as a line to print into the label.


## Building

Just type make.

Requirements are librsvg-2, pangocairo and popt.

The compiler needs to support C23 for constexpr.

## Labels

Labels are created as rectangles (optionally with rounded corners) with the text inside. They can be optionally rotated by 90, 180 or 270 degrees as needed for printing. Text is centered inside the label. The font size is automatically scaled to fit the height, unless explicitly given with -p.

    ./label -o example1.png -r 90 Example
    ./label -o example2.png -r 90 -f Sans Example\ 1 Example\ 2

![Example 1](example1.png "Example 1")
![Example 2](example2.png "Example 2")


