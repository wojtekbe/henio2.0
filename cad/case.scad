// vim: ts=2:sw=2
include <common.scad>
use <front_glass.scad>
use <knob.scad>

arduino_pos = [13, 22, 4];
encoder_pos = [ 0,  0, 4];
encoder_thread_h = 15;
case_h = (encoder_thread_h + encoder_pos.z - get_front_glass_height() - 1);

module led_ring(l)
{
  d1 = 66;
  d2 = 54;
  color("Black") linear_extrude(1) difference() {
    circle(d=d1);
    circle(d=d2);
  }
  r = d1/2 + (d2-d1)/4;
  color("White") translate([0, 0, 1]) for(i=[0:(l-1)]) {
    polar_translate([r, i*360/l]) linear_extrude(2.5) square([4.5, 4.5], center=true);
  }
}

module encoder()
{
  color("DarkBlue") translate([-4, 0, 0]) linear_extrude(2) difference() {
    square(encoder_dims, center=true); // PCB
                                       //mounting_wholes(board_dims=[32,16], $fn=16);
  }
  color("Gray") {
    linear_extrude(8) square([14, 12], center=true); // rotor box
    linear_extrude(15) circle(d=7); // thread
    linear_extrude(17) circle(d=3); // axle
    translate([0, 0, 16]) encoder_shaft(); // shaft
  }
}

module arduino_nano()
{
  color("DarkSlateGray") linear_extrude(1.5) difference() {
    square(arduino_dims, center=true); // PCB
                                       //mounting_wholes(board_dims=[45,18], $fn=16);
  }
  color("Gray") translate([(45/2-2.5), 0, 1.5]) linear_extrude(2.5) square([6, 8], center=true); // USB connector
}

module micro_usb_whole(A, B)
{
  dim_l = [2.5, 7];
  dim_b = [  3, 9];
  rotate([180, 0, 0]) translate([0, 0, -A]) linear_extrude(A, scale=[dim_b.x/dim_l.x, dim_b.y/dim_l.y]) minkowski() {
    square([dim_l.x-1, dim_l.y-1], center=true);
    circle(1, $fn=60);
  }
  translate([0, 0, A]) linear_extrude(B) minkowski() {
    square([dim_l.x-1, dim_l.y-1], center=true);
    circle(1, $fn=60);
  }
}

module case(D, H)
{
  difference() {
    // outer box
    linear_extrude(H) base(D, $fn=36);
    // arduino
    translate(arduino_pos) translate([0, 0, -2]) linear_extrude(H) offset(r=0.5) projection() arduino_nano();
    // micro USB whole
    translate([D/2, arduino_pos.y, arduino_pos.z+1.5+(2.5/2)]) rotate([0, -90, 0]) micro_usb_whole(1, 3);
    // encoder
    translate(encoder_pos) translate([0, 0, -2]) linear_extrude(H) offset(r=0.3) projection() encoder();
    // led ring
    translate([0, 0, H-3.5]) linear_extrude(H) offset(r=-4) base(D, $fn=60);
    // inner round whole
    translate([0, 0, 6]) linear_extrude(H) circle(d=53);
    // arduino mounting wholes
    translate(arduino_pos) rotate([180, 0, 0]) linear_extrude(3) mounting_wholes(board_dims=arduino_dims, $fn=16, dia=1.5);
    // encoder mounting wholes
    translate(encoder_pos) translate([-4, 0, 0]) rotate([180, 0, 0]) linear_extrude(3) mounting_wholes(board_dims=encoder_dims, $fn=16, dia=1.5);
    // front glass mounting wholes
    translate([0, 0, H]) rotate([180, 0, 0]) linear_extrude(3) whole_pattern((D-4)/2, 1.5, 4, $fn=16);
  }

  // PCBs mounting sockets
  translate(arduino_pos) translate([0,  0, -2]) mounting_sockets(h=2, board_dims=arduino_dims, $fn=16, dia=1.5, pads=false);
  translate(encoder_pos) translate([-4, 0, -2]) mounting_sockets(h=2, board_dims=encoder_dims, $fn=16, dia=1.5, pads=false);
}

// case
case(75, case_h);
