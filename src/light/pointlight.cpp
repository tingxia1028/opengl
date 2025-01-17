#include "pointlight.h"
#include "../renderengine/render.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
PointLight::PointLight(const glm::vec3 &ambient, const glm::vec3 &diffuse,
                       const glm::vec3 &specular, const LightType lightType,
                       const glm::vec3 &position, float constTerm,
                       float linearTerm, float quadraticTerm)
    : Light(position, ambient, diffuse, specular, lightType),
      constTerm(constTerm), linearTerm(linearTerm),
      quadraticTerm(quadraticTerm) {
  genShadowMap();
}

void PointLight::configure(ShaderProgram &shaderProgram, std::string lightType,
                           std::string index) {
  Light::configure(shaderProgram, lightType, index);
  shaderProgram.uniformSetFloat(lightType + "s[" + index + "].constant",
                                constTerm);
  shaderProgram.uniformSetFloat(lightType + "s[" + index + "].linear",
                                linearTerm);
  shaderProgram.uniformSetFloat(lightType + "s[" + index + "].quadratic",
                                quadraticTerm);
  shaderProgram.uniformSetVec3F(lightType + "s[" + index + "].position",
                                position);
  shaderProgram.uniformSetFloat(lightType + "s[" + index + "].farPlane",
                                farPlane);
}

void PointLight::genShadowMap() {
  glGenFramebuffers(1, &shadowMapFBO);
  // create depth cubemap texture
  glGenTextures(1, &depthMapTex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, depthMapTex);
  for (unsigned int i = 0; i < 6; ++i)
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  // attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapTex, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Error loading the pointLight Depth Framebuffer" << std::endl;
    return;
  }

  // unbind
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void PointLight::configureShadowMatrices(ShaderProgram &shaderProgram) {
  float nearPlane = 0.01f;
  farPlane = 5.0f;
  glm::mat4 shadowProj = glm::perspective(
      glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
      nearPlane, farPlane);
  std::vector<glm::mat4> shadowTransforms;
  shadowTransforms.push_back(
      shadowProj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, -1.0f, 0.0f)));
  shadowTransforms.push_back(
      shadowProj * glm::lookAt(position,
                               position + glm::vec3(-1.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, -1.0f, 0.0f)));
  shadowTransforms.push_back(
      shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f)));
  shadowTransforms.push_back(
      shadowProj * glm::lookAt(position,
                               position + glm::vec3(0.0f, -1.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, -1.0f)));
  shadowTransforms.push_back(
      shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f),
                               glm::vec3(0.0f, -1.0f, 0.0f)));
  shadowTransforms.push_back(
      shadowProj * glm::lookAt(position,
                               position + glm::vec3(0.0f, 0.0f, -1.0f),
                               glm::vec3(0.0f, -1.0f, 0.0f)));
  for (unsigned int i = 0; i < 6; ++i) {
    shaderProgram.uniformSetMat4("shadowMatrices[" + std::to_string(i) + "]",
                                 shadowTransforms[i]);
  }

  shaderProgram.uniformSetFloat("far", farPlane);
  shaderProgram.uniformSetVec3F("lightPos", position);
}

void PointLight::activeShadowTex() {
  glActiveTexture(GL_TEXTURE0 + depthMapIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, depthMapTex);
}
