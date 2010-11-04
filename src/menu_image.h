/* image2c image dump (toggle_on.png) */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[10*10];
} img_toggle_on = {
  10, 10, 1,
  {
    2,2,2,2,2,2,2,2,2,2,
    2,3,3,3,3,3,3,3,3,0,
    2,3,1,1,1,1,1,1,1,0,
    2,3,1,1,1,1,1,1,1,0,
    2,3,1,1,1,1,1,1,1,0,
    2,3,1,1,1,1,1,1,1,0,
    2,3,1,1,1,1,1,1,1,0,
    2,3,1,1,1,1,1,1,1,0,
    2,3,1,1,1,1,1,1,1,0,
    2,0,0,0,0,0,0,0,0,0
  }
};
/* image2c image dump (toggle_off.png) */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[10*10];
} img_toggle_off = {
  10, 10, 1,
  {
    0,0,0,0,0,0,0,0,0,3,
    0,1,1,1,1,1,1,1,1,3,
    0,1,1,1,1,1,1,1,2,3,
    0,1,1,1,1,1,1,1,2,3,
    0,1,1,1,1,1,1,1,2,3,
    0,1,1,1,1,1,1,1,2,3,
    0,1,1,1,1,1,1,1,2,3,
    0,1,1,1,1,1,1,1,2,3,
    0,1,2,2,2,2,2,2,2,3,
    3,3,3,3,3,3,3,3,3,3
  }
};
/* image2c image dump (radio_on.png) */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[11*11];
} img_radio_on = {
  11, 11, 1,
  {
    5,5,5,5,5,2,5,5,5,5,5,
    5,5,5,5,2,2,2,5,5,5,5,
    5,5,5,2,2,3,2,2,5,5,5,
    5,5,2,2,3,1,3,2,2,5,5,
    5,2,2,3,1,1,1,3,2,2,5,
    2,2,3,1,1,1,1,1,3,2,2,
    5,0,0,4,1,1,1,4,0,0,5,
    5,5,0,0,4,1,4,0,0,5,5,
    5,5,5,0,0,4,0,0,5,5,5,
    5,5,5,5,0,0,0,5,5,5,5,
    5,5,5,5,5,0,5,5,5,5,5
  }
};
/* image2c image dump (radio_off.png) */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[11*11];
} img_radio_off = {
  11, 11, 1,
  {
    5,5,5,5,5,0,5,5,5,5,5,
    5,5,5,5,0,0,0,5,5,5,5,
    5,5,5,0,0,1,0,0,5,5,5,
    5,5,0,0,1,1,1,0,0,5,5,
    5,0,0,1,1,1,1,1,0,0,5,
    0,0,1,1,1,1,1,1,1,0,0,
    5,3,2,2,1,1,1,2,2,3,5,
    5,5,3,2,2,1,2,2,3,5,5,
    5,5,5,3,2,2,2,3,5,5,5,
    5,5,5,5,3,2,3,5,5,5,5,
    5,5,5,5,5,3,5,5,5,5,5
  }
};

/* image2c image dump (arrow_down.png) */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[9*9];
} img_arrow_down = {
  9, 9, 1,
  {
    1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,2,
    0,1,1,1,1,1,1,2,3,
    0,1,1,1,1,1,1,2,3,
    1,0,1,1,1,1,2,3,1,
    1,1,0,1,1,2,3,1,1,
    1,1,1,0,1,3,1,1,1,
    1,1,1,1,0,1,1,1,1,
    1,1,1,1,1,1,1,1,1
  }
};

static struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[9*9];
} img_arrow_up = {
  9, 9, 1,
  {
    1,1,1,1,1,1,1,1,1,
    1,1,1,1,0,1,1,1,1,
    1,1,1,0,1,3,1,1,1,
    1,1,0,1,1,2,3,1,1,
    1,0,1,1,1,1,2,3,1,
    0,1,1,1,1,1,1,2,3,
    0,1,2,2,2,2,2,2,3,
    1,3,3,3,3,3,3,3,2,
    1,1,1,1,1,1,1,1,1
  }
};
