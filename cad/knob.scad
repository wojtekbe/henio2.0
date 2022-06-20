// vim: ts=2:sw=2
include <common.scad>

module encoder_shaft_socket(length=15, delta)
{
  linear_extrude(15) difference() {
    circle(d=6+(2*delta));
    translate([3+delta, 0, 0]) square([3, 6], center=true);
  }
  linear_extrude(5) circle(d=7, $fn=60);
  linear_extrude(2) circle(d=11, $fn=60);
}

module knob(dia, height)
{
  bevel = .8;
  dia_b = dia;
  dia_t = dia-2;

  difference() {
    union() {linear_extrude(height - bevel, scale=(dia_t/dia_b)) circle(d=dia);
      translate([0, 0, height - bevel]) linear_extrude(bevel, scale=((dia_t-(bevel*2))/dia_t)) circle(d=dia_t);
    }
    *translate([(dia/2)-6, 0, height+4]) sphere(5); // finger half-sphere
    encoder_shaft_socket(delta=0.1); // shaft socket
  }
}

rotate([180, 0, 0]) translate([0, 0, -16]) !knob(dia=35, height=16, $fn=240);
