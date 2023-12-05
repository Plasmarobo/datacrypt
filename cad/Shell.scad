// Datacrypt

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
        translate([0,0, -0.1]) cylinder(h + 0.2, r = w, $fa = 4);
    }
}

module mobo() {
    W = 177.8;
    H = 116.7;
    MOUNT_INSET = 6.190;
    DISPLAY_X = 20.320;
    DISPLAY_Y = 22.225;
    DISPLAY_PITCH_X = 40.640;
    DISPLAY_W = 22.860;
    DISPLAY_H = 13.716;
    DISPLAY_Z = 13.7;
    for(i = [0:3])
    {
        translate([(i * DISPLAY_PITCH_X) + DISPLAY_X, DISPLAY_Y, 2])
        cube([DISPLAY_W, DISPLAY_H, DISPLAY_Z]);
    }
    
    SUB_DISPLAY_X = 20.955;
    SUB_DISPLAY_Y = 55.880;
    SUB_DISPLAY_PITCH_X = 40.640;
    SUB_DISPLAY_W = 22.479;
    SUB_DISPLAY_H = 5.461;
    SUB_DISPLAY_Z = 13.7;
    for(i = [0:3])
    {
        translate([(i * SUB_DISPLAY_PITCH_X) + SUB_DISPLAY_X, SUB_DISPLAY_Y, 2])
        cube([SUB_DISPLAY_W, SUB_DISPLAY_H, SUB_DISPLAY_Z]);
    }
    LED_W = 5.080;
    LED_H = 5.080;
    LED_DISP_X = 30.480;
    LED_DISP_Y = 45.720;
    LED_DISP_PITCH_X = 40.640;
    LED_Z = 13.7;
    for(i = [0:3])
    {
        translate([(i * LED_DISP_PITCH_X) + LED_DISP_X,LED_DISP_Y,2])
        cube([LED_W, LED_H, LED_Z]);
    }
    
    COUNTERS_X = 68.2;
    COUNTERS_1_Y = 71.247;
    COUNTERS_2_Y = 81.534;
    TIMER_Y = 96.774;
    COUNTER_PITCH_X = 15.24;
    TIMER_PITCH_X = 10.160;
    for(i = [0:3])
    {
        translate([(i * COUNTER_PITCH_X) + COUNTERS_X, COUNTERS_1_Y, 2])
        cube([LED_W, LED_H, LED_Z]);
    }
    for(i = [0:3])
    {
        translate([(i * COUNTER_PITCH_X) + COUNTERS_X, COUNTERS_2_Y, 2])
        cube([LED_W, LED_H, LED_Z]);
    }
    for(i = [0:5])
    {
        translate([(i * TIMER_PITCH_X) + COUNTERS_X - TIMER_PITCH_X/2, TIMER_Y, 2])
        cube([LED_W, LED_H, LED_Z]);
    }
    for(x = [MOUNT_INSET, MOUNT_INSET + 165.669])
        for(y = [MOUNT_INSET, MOUNT_INSET + 104.811])
            translate([x, y, - 3.5])
            cylinder(h = 3.6, r = 2.75);
    cube([W,H,2]);
}

module base_lower(m)
{
    MARGIN = m;
    BASE_W = 177.8;
    BASE_H = 116.7;
    CASE_W = 177.8 + (2 * MARGIN);
    CASE_H = 116.7 + (2 * MARGIN);
    MOUNT_INSET = 6.190;
    difference()
    {
        translate([-MARGIN, -MARGIN]) cube([CASE_W, CASE_H, 5]);
        translate([CASE_W-(2 * MARGIN), CASE_H-(2 * MARGIN), -0.1]) roundout(MARGIN + 0.1,5.2);
        translate([0, CASE_H-(2 * MARGIN), -0.1]) rotate([0,0,90]) roundout(MARGIN + 0.1,5.2);
        translate([CASE_W-(2 * MARGIN), 0, -0.1]) rotate([0,0,-90]) roundout(MARGIN + 0.1,5.2);
        translate([0, 0, -0.1]) rotate([0,0,180]) roundout(MARGIN + 0.1,5.2);
        translate([BASE_W+MARGIN/2.5, BASE_H+MARGIN/2.5, 2]) m3_well(10);
        translate([-MARGIN/2.5, BASE_H+MARGIN/2.5, 2]) m3_well(10);
        translate([BASE_W+MARGIN/2.5, -MARGIN/2.5, 2]) m3_well(10);
        translate([-MARGIN/2.5, -MARGIN/2.5, 2]) m3_well(10);
    }
}

module base_upper(m)
{
    MARGIN = m;
    BASE_W = 177.8;
    BASE_H = 116.7;
    CUTOUT = 75;
    CASE_W = BASE_W + (2 * MARGIN);
    CASE_H = BASE_H + (2 * MARGIN);
    difference()
    {
        translate([-MARGIN, -MARGIN, 5]) cube([CASE_W, CASE_H, 12]);
        translate([0,0,4]) cube([BASE_W, BASE_H, 11]);
        translate([0,0,4]) lower_bezel();
        translate([-MARGIN, -MARGIN]) cube([CASE_W, CASE_H, 5]);
        translate([CASE_W-(2 * MARGIN), CASE_H-(2 * MARGIN), -0.1]) roundout(MARGIN + 0.1,17.2);
        translate([0, CASE_H-(2 * MARGIN), -0.1]) rotate([0,0,90]) roundout(MARGIN + 0.1,17.2);
        translate([CASE_W-(2 * MARGIN), 0, -0.1]) rotate([0,0,-90]) roundout(MARGIN + 0.1,17.2);
        translate([0, 0, -0.1]) rotate([0,0,180]) roundout(MARGIN + 0.1,17.2);
        translate([BASE_W+MARGIN/2.5, BASE_H+MARGIN/2.5, 0]) m3_well(10);
        translate([-MARGIN/2.5, BASE_H+MARGIN/2.5, 0]) m3_well(10);
        translate([BASE_W+MARGIN/2.5, -MARGIN/2.5, 0]) m3_well(10);
        translate([-MARGIN/2.5, -MARGIN/2.5, 0]) m3_well(10);
        translate([(BASE_W / 2) - (CUTOUT / 2), BASE_H-5, 0]) cube([CUTOUT, 30, 10]);
    }
    
    translate([(BASE_W / 2) - CUTOUT, BASE_H+20, 5]) {
        difference()
        {
            union()
            {
                cube([10,5,12]);
                translate([0, 5, 6.03]) rotate([0,90,0]) cylinder(h=10, r=6.03);
            }
            translate([-0.1, 5, 6.03]) rotate([0,90, 0]) cylinder(h=10.2, r=3.6);
        }
    }
    
    translate([(BASE_W / 2) + CUTOUT, BASE_H+20, 5]) {
        difference()
        {
            union()
            {
                cube([10,5,12]);
                translate([0, 5, 6.03]) rotate([0,90,0]) cylinder(h=10, r=6.03);
            }
            translate([-0.1, 5, 6.03]) rotate([0,90, 0]) cylinder(h=10.2, r=3.6);
        }
    }
        
}

module lower_base()
{
    MARGIN = 20;
    difference()
    {
        base_lower(MARGIN);
        translate([0,0,4]) mobo();
    }
}


module lower_bezel()
{
    MARGIN = 4;
    CASE_W = 177.8 + (2 * MARGIN);
    CASE_H = 116.7 + (2 * MARGIN);
    BEZEL_W = 2;
    difference()
    {
        translate([-MARGIN, -MARGIN]) cube([CASE_W, CASE_H, 8]);
        translate([0,0,4]) mobo();
        translate([-MARGIN+BEZEL_W, -MARGIN+BEZEL_W, -0.5]) cube([CASE_W-(BEZEL_W * 2),CASE_H-(BEZEL_W * 2), 9]);
    }
}

module upper_base()
{
    MARGIN = 20;
    difference()
    {
        base_upper(MARGIN);
        translate([0,0,4]) mobo();
    }
}

module m3_well(h)
{
    INF_PUNCH = 999;
    cylinder(h = h, r = 1.7, $fn = 20); 
    translate([0,0,h]) cylinder(h = INF_PUNCH, r = 3, $fn = 20);
    translate([0,0,-INF_PUNCH]) linear_extrude(height=INF_PUNCH) scale([3.05, 3.05, 0]) polygon([[1, 0], [0.5, 0.86603], [-0.5, 0.86603], [-1, 0], [-0.5, -0.86603], [0.5, -0.86603]]);
}

module toggle()
{
    // 12 mm hole
    // 36mm? depth
    cylinder(h = 36, r = 12 / 2);
}

module power_switch()
{
    // 19 x 13 mm
    cube([19, 13, 35]);
}

module sub_button()
{
    // 16mm button
    // Internal depth of 24mm?
    cylinder(h = 36, r = 16 / 2);
}

module switches()
{
    translate([5,0,10])
    {
        // Lock row
        translate([0, 45, 0])
        {
           for(x = [0:3])
            translate([x * 35, 0, 0])
           toggle();       
        }
        
        translate([140,50, 0])
        {
            for (x =[0:1])
                translate([x * 35, 0, 0])
            toggle();
        }
        
        translate([70,110,0])
        for(x = [0:3])
            translate([x * 30, 0 ,0])
            sub_button();
        translate([0, 95, 0])
        for(x = [0:1])
            for(y = [0:1])
                translate([x * 30, y * 30, 0])
            sub_button();
            
        translate([190,115,0]) rotate([90,0,0]) power_switch();
        translate([45,0,20]) cube([80,20,10]);
    }
}

module switchboard_top()
{
    W = 240;
    L = 140;
    H = 45;
    MARGIN = 7;
    difference()
    {
    union()
    {
        translate([-30,5,0])
        {
            difference()
            {            
                cube([W,L,H]);
                translate([MARGIN, MARGIN, 0])
                {
                    translate([W-(2 * MARGIN), L-(2 * MARGIN), -0.1]) roundout(MARGIN + 0.1,H + 0.2);
                    translate([0, L-(2 * MARGIN), -0.1]) rotate([0,0,90]) roundout(MARGIN + 0.1,H + 0.2);
                    translate([W-(2 * MARGIN), 0, -0.1]) rotate([0,0,-90]) roundout(MARGIN + 0.1,H + 0.2);
                    translate([0, 0, -0.1]) rotate([0,0,180]) roundout(MARGIN + 0.1,H + 0.2);
                    translate([0,0,-0.1]) cube([W-(2 * MARGIN), L - (2 * MARGIN), H-4]);
                }
            }
        }   
    }
    switches();
    }
}

module switchboard_bottom()
{
    W = 240;
    L = 140;
    H = 5;
    MARGIN = 7;
    translate([-30,5,0])
    {
        difference()
        {            
            cube([W,L,H]);
            translate([MARGIN, MARGIN, 0])
            {
                translate([W-(2 * MARGIN), L-(2 * MARGIN), -0.1]) roundout(MARGIN + 0.1,H + 0.2);
                translate([0, L-(2 * MARGIN), -0.1]) rotate([0,0,90]) roundout(MARGIN + 0.1,H + 0.2);
                translate([W-(2 * MARGIN), 0, -0.1]) rotate([0,0,-90]) roundout(MARGIN + 0.1,H + 0.2);
                translate([0, 0, -0.1]) rotate([0,0,180]) roundout(MARGIN + 0.1,H + 0.2);
            }
        }
            translate([MARGIN,MARGIN,H])
            {
                
        difference()
        {
                cube([W - (2 * MARGIN), L - (2 * MARGIN), H]);
                translate([MARGIN,MARGIN,0.1]) cube([W - (4 * MARGIN), L - (4 * MARGIN), H]);
            }
        }
    }   
}

module lower_part()
{
    union()
    {
        lower_base();
        lower_bezel();
    }
}

// Top part
//rotate([0,180,0]) upper_base();
//lower_part();

// Bottom part
//rotate([0,180,0]) switchboard_top();
switchboard_bottom();

