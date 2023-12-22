// Datacrypt
include <minkowskiRound.scad>


module prism(l, w, h){
polyhedron(//pt 0        1        2        3        4        5
      points=[[0,0,0], [l,0,0], [l,w,0], [0,w,0], [0,w,h], [l,w,h]],
      faces=[[0,1,2,3],[5,4,3,2],[0,4,5,1],[0,3,4],[5,2,1]]
      );
}

module roundout(w,h)
{
    difference()
    {
        cube([w,w,h]);
        translate([0,0, -0.1]) cylinder(h + 0.2, r = w, $fn = 32);
    }
}

module rounded_box(w,l,h,r)
{
    difference()
    {
        cube([w, l, h]);
        translate([w-r+0.1, l-r+0.1, -0.1]) roundout(r,h + 0.2);
        translate([r-0.1, l-r+0.1, -0.1]) rotate([0,0,90]) roundout(r,h + 0.2);
        translate([w-r+0.1, r-0.1, -0.1]) rotate([0,0,-90]) roundout(r,h + 0.2);
        translate([r-0.1, r-0.1, -0.1]) rotate([0,0,180]) roundout(r,h + 0.2);
    }
}

INF_PUNCH = 999;

module m3_well(h)
{
    cylinder(h = h, r = 1.7, $fn = 20); 
    translate([0,0,h]) cylinder(h = INF_PUNCH, r = 3, $fn = 20);
    translate([0,0,-INF_PUNCH]) linear_extrude(height=INF_PUNCH) scale([3.3, 3.3, 0]) polygon([[1, 0], [0.5, 0.86603], [-0.5, 0.86603], [-1, 0], [-0.5, -0.86603], [0.5, -0.86603]]);
}

module m3_standoff(h)
{
    difference()
    {
        cylinder(h = h, d = 8, $fn = 32);
        translate([0,0,-0.1]) cylinder(h = h + 0.2, d=3.7, $fn = 16);
        translate([0,0,h/2+0.1]) cylinder(h = h / 2, d = 6.1, $fn = 32);
    }
}

BOARD_PAD = 1;
BOARD_W = 178;
BOARD_L = 120;
BOARD_H = 1.6;
BOARD_Z_CLEAR = 7.5;
// Hight of mount pillars, to clear battery and connectors

BOARD_MOUNT_CLEAR = 15;
// On center
MOUNTS = [[6.190, 6.005,0],[171.859, 6.005,0],[171.859, 110.816,0],[6.190, 110.816,0]];
// Top left
DISPLAY64_W = 27.5;
DISPLAY64_L = 15.25; 
DISPLAY64_L_OFFSET = 4.2;

DISPLAY32_W = 27.5;
DISPLAY32_L = 10;
DISPLAY32_W_OFFSET = 5.75;
DISPLAY_H = BOARD_Z_CLEAR;

DISPLAYS_64 =[[18.04,100.22,0],[58.68,100.22,0],[99.310, 100.22,0],[139.94, 100.22,0]];
DISPLAYS_32 = [[14.00,63.430,0],[54.64, 63.430,0],[95.240, 63.430,0],[135.39, 63.430,0]];

LED_MARGIN = 0.15;
LED_W = 5.080;
LED_L = 5.080;
LED_DIFFUSE_H = 1;
LED_DIFFUSE_WALL = 1;
LED_H = BOARD_Z_CLEAR - LED_DIFFUSE_H;
LEDS = [
    [30.52,70.99,0],[71.16, 70.99,0],[111.81,70.99,0],[152.42,70.99,0],
    [68.1, 45.46,0], [83.85,45.46,0], [98.55,45.46,0], [113.84, 45.46,0],
    [68.1,35.34,0], [83.85,35.34,0], [98.55,35.34,0], [113.84,35.34,0],
    [68.08,20.09,0],[78.29, 20.09,0],[88.45, 20.09,0],[98.56, 20.09,0],[108.75, 20.09,0],[118.88, 20.09,0]
];
USB_W = 10.32;
USB_L = 10;
USB_H = 3.5;
USB_W_OFFSET = 5;
USB = [-USB_W_OFFSET, 34.96,0];
BUS_CONN = [58.44,10.09, 0];
BUS_CONN_W = 50.8;
BUS_CONN_L = 5.08;
BUS_MARGIN = 1;

module display_cutouts()
{
    well_offset = 2;
    for(pos = MOUNTS)
    {
        translate(pos) translate([0,0,-BOARD_MOUNT_CLEAR]) m3_well(BOARD_MOUNT_CLEAR+well_offset);
    }
    
    for(pos = DISPLAYS_64)
    {
        translate(pos) translate([0,-DISPLAY64_L-DISPLAY64_L_OFFSET,0]) cube([DISPLAY64_W,DISPLAY64_L, DISPLAY_H]);
    }
    for(pos = DISPLAYS_32)
    {
        translate(pos) translate([DISPLAY32_W_OFFSET,-DISPLAY32_L, 0]) cube([DISPLAY32_W, DISPLAY32_L, DISPLAY_H]);
    }
    for(pos = LEDS)
    {
        translate(pos) translate([-LED_MARGIN, -(LED_L+LED_MARGIN), 0]) cube([LED_W+(2 * LED_MARGIN), LED_L+(2*LED_MARGIN), LED_H]);
    }
    translate(USB) translate([0, -USB_L,0]) cube([USB_W + USB_W_OFFSET, USB_L, USB_H]);
    translate([-BOARD_PAD,-BOARD_PAD,-BOARD_MOUNT_CLEAR]) cube([BOARD_W+(2*BOARD_PAD), BOARD_L+(2*BOARD_PAD), BOARD_MOUNT_CLEAR]);
    // General cutout
    difference()
    {
        translate([-BOARD_PAD,-BOARD_PAD,0]) cube([BOARD_W+(2*BOARD_PAD), BOARD_L+(2*BOARD_PAD), BOARD_Z_CLEAR-2]);
        for(pos = LEDS)
        {
            translate(pos) translate([-LED_DIFFUSE_WALL, -LED_L-LED_DIFFUSE_WALL, 0]) cube([LED_W+(2*LED_DIFFUSE_WALL), LED_L+(2*LED_DIFFUSE_WALL), LED_H]);
        }
    }
    translate(BUS_CONN) translate([-BUS_MARGIN,-BUS_MARGIN,-20]) cube([BUS_CONN_W+(2*BUS_MARGIN), BUS_CONN_L + (2*BUS_MARGIN), 20]);
}

DISPLAY_CASE_W = 184;
DISPLAY_CASE_L = 132;
DISPLAY_CASE_H = BOARD_Z_CLEAR + BOARD_MOUNT_CLEAR-0.1;
DISPLAY_BACK_H = 2;
DISPLAY_GUIDE_WIDTH = 0.5;

module display()
{
    x_offset = ((DISPLAY_CASE_W - BOARD_W) / 2);
    y_offset = ((DISPLAY_CASE_L - BOARD_L) / 2);
    z_offset = BOARD_MOUNT_CLEAR-0.1;
    exp_fit = 0.15;
    difference()
    {
        rounded_box(DISPLAY_CASE_W, DISPLAY_CASE_L, DISPLAY_CASE_H,3);
        translate([x_offset, y_offset, z_offset]) color([1.0,0,0,0.3]) display_cutouts();
        
        translate([(BOARD_W/4)-exp_fit,
                    y_offset-DISPLAY_GUIDE_WIDTH-exp_fit*0.99,
                    0])
       cube([(BOARD_W / 2)+(2 * exp_fit),
            DISPLAY_GUIDE_WIDTH+exp_fit,
            BOARD_Z_CLEAR-4]);
        
        translate([(BOARD_W/4)-exp_fit,
                    DISPLAY_CASE_L-y_offset,
                    0])
       cube([(BOARD_W / 2)+(2 * exp_fit),
            DISPLAY_GUIDE_WIDTH+exp_fit,
            BOARD_Z_CLEAR-4]);
        
        translate([x_offset-DISPLAY_GUIDE_WIDTH-exp_fit*0.99,
                    (BOARD_L/4)-exp_fit,
                    0])
        cube([DISPLAY_GUIDE_WIDTH+exp_fit,
             (BOARD_L/2)+(2 * exp_fit),
             BOARD_Z_CLEAR-4]);
        
        translate([DISPLAY_CASE_W-x_offset-exp_fit,
                    (BOARD_L/4)-exp_fit,
                    0])
       cube([DISPLAY_GUIDE_WIDTH+exp_fit,
            (BOARD_L/2)+(2*exp_fit),
            BOARD_Z_CLEAR-4]);
            
       translate([30.9,0.7-0.1,(DISPLAY_CASE_H/2)+0.1]) rotate([0,0,-45])cube(center=true,[3,5.6,DISPLAY_CASE_H+0.3]);
       translate([DISPLAY_CASE_W-(30.9), 0.7-0.1,(DISPLAY_CASE_H/2)+0.1]) rotate([0,0,45]) cube(center=true,[3,5.6,DISPLAY_CASE_H+0.3]);
    
    }
    for(pos = MOUNTS)
    {
        translate(pos) translate([x_offset, y_offset, z_offset]) m3_standoff(BOARD_Z_CLEAR-0.9);
    }
}

module display_back()
{
    x_offset = ((DISPLAY_CASE_W - BOARD_W) / 2);
    y_offset = ((DISPLAY_CASE_L - BOARD_L) / 2);
    z_offset = BOARD_MOUNT_CLEAR-0.1+1;
    difference()
    {
        rounded_box(DISPLAY_CASE_W, DISPLAY_CASE_L, DISPLAY_BACK_H,3);
        translate([x_offset, y_offset, z_offset]) color([1.0,0,0,0.3]) display_cutouts();
        translate([30.9,0.7-0.1,(DISPLAY_CASE_H/2)+0.1]) rotate([0,0,-45])cube(center=true,[3,5.6,DISPLAY_CASE_H+0.3]);
        translate([DISPLAY_CASE_W-(30.9), 0.7-0.1,(DISPLAY_CASE_H/2)+0.1]) rotate([0,0,45]) cube(center=true,[3,5.6,DISPLAY_CASE_H+0.3]);
    }
    for(pos = MOUNTS)
    {
        translate(pos) translate([x_offset, y_offset, 1]) difference()
        {
            m3_standoff(BOARD_MOUNT_CLEAR-1.6);
            translate([0,0,2]) m3_well(9);
        }
    }
    translate([BOARD_W/4,y_offset-DISPLAY_GUIDE_WIDTH,0]) cube([BOARD_W / 2, DISPLAY_GUIDE_WIDTH, BOARD_Z_CLEAR-4]);
    translate([BOARD_W/4,DISPLAY_CASE_L-y_offset,0]) cube([BOARD_W / 2, DISPLAY_GUIDE_WIDTH, BOARD_Z_CLEAR-4]);
    translate([x_offset-DISPLAY_GUIDE_WIDTH, BOARD_L/4,0]) cube([DISPLAY_GUIDE_WIDTH, BOARD_L/2, BOARD_Z_CLEAR-4]);
    translate([DISPLAY_CASE_W-x_offset,BOARD_L/4,0]) cube([DISPLAY_GUIDE_WIDTH, BOARD_L/2, BOARD_Z_CLEAR-4]);
}

LATCH_ID = 3.6;
LATCH_OD = 8;

module latch_base(l,ext=1)
{
    difference()
    {
        union()
        {
            cylinder(h=l, d=LATCH_OD, $fn=36);
            translate([-LATCH_OD/2,-LATCH_OD/2-ext,0]) cube([LATCH_OD,LATCH_OD/2+ext,l]);
        }
        translate([0,0,-0.1]) cylinder(h=l+0.2, d=LATCH_ID, $fn=36);
    }
}

module latch_outer(l,ext=1)
{
    // Cut out the middle
    difference()
    {
        latch_base(l, ext);
        translate([-(LATCH_OD/2)-0.1, -(LATCH_OD/2)-0.1-ext, l/3])cube([LATCH_OD+0.2, LATCH_OD+0.2+ext, l/3]);
        translate([0,0,(5*l)/6]) linear_extrude(height=INF_PUNCH) scale([3.05, 3.05, 0]) polygon([[1, 0], [0.5, 0.86603], [-0.5, 0.86603], [-1, 0], [-0.5, -0.86603], [0.5, -0.86603]]);
    }
}

module latch_inner(l, ext=1)
{
    // Cut off the ends
    difference()
    {
        latch_base(l,ext);
        translate([-(LATCH_OD/2)-0.1, -(LATCH_OD/2)-0.1-ext, -0.1])cube([LATCH_OD+0.2, LATCH_OD+0.2+ext, l/3]);
        translate([-(LATCH_OD/2)-0.1, -(LATCH_OD/2)-0.1-ext, ((2 * l) / 3)+0.1])cube([LATCH_OD+0.2, LATCH_OD+0.2+ext, l/3]);
    }
}

// Preset for M3 short
module heat_set(h=4,d=4.8)
{
    translate([0,0,-(h-0.1)]) cylinder(h=h,d=d, $fn=36);
}

module toggle()
{
    // 12 mm hole
    // 36mm? depth
    cylinder(h = 36, r = 12.6 / 2);
    translate([-17.3/2,-27,23-0.8])cube([17.3,40.6,0.9]);
}

module power_switch()
{
    // 19 x 13 mm
    cube([19.3, 12.5, 35]);
}

module button()
{
    // 16mm button
    // Internal depth of 24mm?
    cylinder(h = 36, r = 16.6 / 2);
}

module switches()
{
    translate([5,0,10])
    {
        // Lock row
        translate([28, 85, 0])
        {
           for(x = [0:3])
            translate([x * 25, 0, 0])
           toggle();       
        }
        
        translate([125,35, 0])
        {
            for (x =[0:1])
                translate([x * 20, 0, 0])
            toggle();
        }
        
        translate([30,35,0])
        for(x = [0:3])
            translate([x * 20, 0 ,0])
            button();
        translate([130, 65, 0])
        for(x = [0:1])
            for(y = [0:1])
                translate([x * 20, y * 20, 0])
            button();
    }
}

module switchboard_body()
{
    z_off = 3.4;
    l_off = 5.8;
    difference()
    {
        union()
        {
            // Top block
            translate([0,-DISPLAY_CASE_L-l_off,-0.5]) rotate([6,0,0])
            {
                cube([DISPLAY_CASE_W, DISPLAY_CASE_L+l_off, 20]);
            }
            // Base block
            translate([0,-140,-22]) cube([DISPLAY_CASE_W, 179, 41]);
            // Display rails
            translate([0,5,34]) rotate([75,0,0])
            {
                translate([30.9,0.7-0.15,(DISPLAY_CASE_H/2)+0.1]) rotate([0,0,-45]) translate([0,-1.1,0]) cube(center=true,[2.7,7.6,DISPLAY_CASE_H+0.3]);
                   translate([DISPLAY_CASE_W-(30.9), 0.7-0.15,(DISPLAY_CASE_H/2)+0.1]) rotate([0,0,45]) translate([0,-1.1,0]) cube(center=true,[2.7,7.6,DISPLAY_CASE_H+0.3]);
                translate([0,-30.3,-37]) cube([DISPLAY_CASE_W, 30, 60]);
                // Stop block
                translate([0,-1,-8]) cube([DISPLAY_CASE_W, 3,3]);
            }
            // Back block
            translate([0,38.75,-7.45]) rotate([15,0,0]) cube([DISPLAY_CASE_W, 10,30]);
            translate([0,38,-22]) cube([DISPLAY_CASE_W, 10.4,17.2]);
            
        }
        translate([0,0,-6])
            rotate([6,0,0])
            {
                translate([10,-130,40]) heat_set();
                translate([DISPLAY_CASE_W-10,-130,40]) heat_set();
                translate([10,-23,40]) heat_set();
                translate([DISPLAY_CASE_W-10,-23,40]) heat_set();
            }
        
    }
}

module switchboard_base()
{
    CLIP_W = 28;
    CLIP2_W = 90;
    CLIP3_W = 70;
    difference()
    {
        switchboard_body();
        translate([CLIP_W/2,-135,-20]) cube([DISPLAY_CASE_W-CLIP_W, 102, 80]);
        translate([CLIP2_W/2,-92,-20]) cube([DISPLAY_CASE_W-CLIP2_W, 134, 80]);
        translate([CLIP3_W/2,65,02.5]) rotate([71,0,0]) {
            cube([DISPLAY_CASE_W-CLIP3_W, 52, 90]);
            translate([5,-2.9,80])rotate([90,0,0])heat_set();
            translate([5,-2.9,32])rotate([90,0,0])heat_set();
            translate([DISPLAY_CASE_W-CLIP3_W-5,-2.9,32])rotate([90,0,0])heat_set();
            translate([DISPLAY_CASE_W-CLIP3_W-5,-2.9,80])rotate([90,0,0])heat_set();
        }
    }
}

module sh_well(z)
{
    // M3
    translate([0,0,-INF_PUNCH+z/2+0.1]) cylinder(h=INF_PUNCH, d=3.6, $fn=36);
    translate([0,0,z/2]) cylinder(h=INF_PUNCH, d=6, $fn=36);
}

module switchboard_front_panel()
{
    difference()
    {
        translate([0,4.5,0]) rounded_box(DISPLAY_CASE_W, DISPLAY_CASE_L-11, 3,4);
        translate([0,0,-30]) switches();
        translate([10, 11, 0]) sh_well(1.5);
        translate([10, DISPLAY_CASE_L-14, 0]) sh_well(1.5);
        translate([DISPLAY_CASE_W-10, 11, 0]) sh_well(1.5);
        translate([DISPLAY_CASE_W-10, DISPLAY_CASE_L-14, 0]) sh_well(1.5);
    }
    
}

module switchboard_back_panel()
{
    difference()
    {
        translate([1,1,0])rounded_box(DISPLAY_CASE_W-72, 65, 3,4);
        translate([5, 9, 0]) sh_well(1.5);
        translate([5, 57, 0]) sh_well(1.5);
        translate([DISPLAY_CASE_W-70-5, 9, 0]) sh_well(1.5);
        translate([DISPLAY_CASE_W-70-5, 57, 0]) sh_well(1.5);
        translate([20,40,-10]) power_switch();
    }
}
//rotate([180,0,0]) translate([0,0,-DISPLAY_CASE_H]) display();
//display_back();
//translate([0,5,34]) rotate([75,0,0]) display();
//switchboard_base();
switchboard_front_panel();
//switchboard_back_panel();
//color([0,0,1.0,0.1]) translate([70/2,-19,32]) rotate([71-90,0,0]) switchboard_back_panel();
