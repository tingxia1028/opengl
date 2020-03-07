
#ifndef OPENGL_SCENE_H
#define OPENGL_SCENE_H

#include "../camera/camera.h"
#include "../light/light.h"
#include "model.h"
#include <glad/glad.h>
class Scene {
public:
  Scene() = default;
  ~Scene() = default;
  Scene(std::vector<Model> &models, Camera *camera,
        std::vector<Light *> &lights);
  void cleanUp();

  std::vector<Model> models;
  Camera *camera;
  std::vector<Light *> lights;
};

#endif // OPENGL_SCENE_H