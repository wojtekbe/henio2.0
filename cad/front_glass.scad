// vim: ts=2:sw=2
include <common.scad>

front_glass_h = 2;

function get_front_glass_height() = front_glass_h;

module front_glass(d)
{
  color("White", 0.5) linear_extrude(front_glass_h) difference() {
    base(d, $fn=36*2);
    circle(d=7, $fn=120);
    whole_pattern(r=(d-4)/2, dia=2, num=4, $fn=120);
  }
}

front_glass(d=75);
