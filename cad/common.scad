// vim: ts=2:sw=2
arduino_dims = [45, 18];
encoder_dims = [32, 16];

module polar_translate(v)
{
  d = v.x;
  a = v.y;
  rotate([0, 0, a]) translate([d, 0, 0]) children();
}

module base(D)
{
  circle(d=D);
  square(D/2);
}

module whole_pattern(r, dia, num)
{
  for(i=[0:(num-1)]) polar_translate([r, i*360/num]) circle(d=dia);
}


module pad(a, r)
{
  minkowski() {
    square(a-(2*r), center=true);
    circle(r, $fn=60);
  }
}

module mounting_wholes(dia=2.5, board_dims)
{
  a=(1+2.5/2); // board edge to whole center
  C=[board_dims.x/2-a, board_dims.y/2-a];
  translate([ C.x,  C.y]) circle(d=dia);
  translate([ C.x, -C.y]) circle(d=dia);
  translate([-C.x, -C.y]) circle(d=dia);
  translate([-C.x,  C.y]) circle(d=dia);
}

module mounting_sockets(h, dia=2.5, board_dims, pads=false)
{
  a = (1+2.5/2); // board edge to whole center
  C = [board_dims.x/2-a, board_dims.y/2-a];
  socket_dia = dia + 3;
  linear_extrude(h) difference() {
    union() {
      translate([ C.x,  C.y]) if(pads) pad(socket_dia, 0.8); else circle(d=socket_dia);
      translate([ C.x, -C.y]) if(pads) pad(socket_dia, 0.8); else circle(d=socket_dia);
      translate([-C.x, -C.y]) if(pads) pad(socket_dia, 0.8); else circle(d=socket_dia);
      translate([-C.x,  C.y]) if(pads) pad(socket_dia, 0.8); else circle(d=socket_dia);
    }
    mounting_wholes(dia=dia, board_dims=board_dims);
  }
}

module cog_2d(d, n_teeth, arm_w)
{
  dia = d.x;
  inner_dia = d.y;
  arms_num = 5;

  module arm(w, l) {
    translate([-w/2, 0, 0]) square([w, l]);
  }

  union() {
    // teeth
    difference() {
      circle(d=dia, $fn=n_teeth);
      for(i=[0:(n_teeth-1)]) {
        rotate(i*360/n_teeth) translate([dia/2, 0, 0]) circle(5, $fn=60);
      }
      circle(d=inner_dia, $fn=60);
    }
    // arms
    for(i=[0:(arms_num-1)]) {
      rotate(i*360/arms_num) arm(arm_w, inner_dia/2);
    }
    // inner circle
    circle(d=d.z, $fn=60);
  }
}

module cutz(z)
{
  if (z >= 0) difference() {
    children(0);
    translate([0, 0, z]) linear_extrude(999) square([999, 999], center=true);
  }
  else difference() {
    translate([0, 0, z]) children(0);
    rotate([180, 0, 0]) linear_extrude(999) square([999, 999], center=true);
  }
}


module encoder_shaft()
{
  linear_extrude(15) difference() {
    circle(d=6, $fn=60);
    translate([3 ,0, 0]) square([3, 6], center=true);
  }
  linear_extrude(5) circle(d=7, $fn=60);
}
