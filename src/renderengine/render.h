
#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "rawmodel.h"
class Render {
public:
  void prepare();
  void render(RawModel rawModel);
};

#endif // OPENGL_RENDER_H
