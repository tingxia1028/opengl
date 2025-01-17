
#ifndef OPENGL_POINTLIGHT_H
#define OPENGL_POINTLIGHT_H

#include "light.h"
#include <glm/vec3.hpp>
class PointLight : public Light {
public:
  PointLight(const glm::vec3 &ambient, const glm::vec3 &diffuse,
             const glm::vec3 &specular, const LightType lightType,
             const glm::vec3 &position, float constTerm, float linearTerm,
             float quadraticTerm);
  void configure(ShaderProgram &shaderProgram, std::string lightType,
                 std::string index) override;
  void configureShadowMatrices(ShaderProgram &shaderProgram) override;
  void activeShadowTex() override;

  float constTerm;
  float linearTerm;
  float quadraticTerm;
  float farPlane;

private:
  void genShadowMap() override;
};

#endif // OPENGL_POINTLIGHT_H
