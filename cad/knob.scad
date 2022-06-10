// vim: ts=2:sw=2
include <common.scad>

module knob(dia, height)
{
  bevel = .6;
  segments = 240;
  difference() {
    union() {linear_extrude(height - bevel) circle(d=dia, $fn=segments);
      translate([0, 0, height - bevel]) linear_extrude(bevel, scale=((dia-(bevel*2))/dia)) circle(d=dia, $fn=segments);
    }
    // finger sphere
    translate([(dia/2), 0, height]) sphere(5, $fn=segments);
    encoder_shaft(); // shaft whole
  }
}

knob(dia=45, height=16);
