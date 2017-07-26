#pragma once

/* view + controller 3d */

BEGIN_C

typedef void* vc3d_t;

vc3d_t vc3d_create(app_t* app);
void vc3d_paint(vc3d_t vc, int x, int y, int w, int h);
void vc3d_shape(vc3d_t vc, int x, int y, int w, int h);
void vc3d_input(vc3d_t vc, input_event_t* e);
void vc3d_destroy(vc3d_t vc);

END_C
