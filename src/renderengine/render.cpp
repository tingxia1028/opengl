
#include "render.h"
#include "../light/directionallight.h"
#include "../scene/scene.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <set>

void Render::prepare(Camera *camera, DisplayManager &displayManager) {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // uniform buffer object
  if (camera) {
    GLuint uboMatrices;
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL,
                 GL_STATIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0,
                      2 * sizeof(glm::mat4));
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                    glm::value_ptr(camera->getProjectionMatrix(true)));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                    glm::value_ptr(camera->getViewMatrix()));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }
  glViewport(0, 0, 2 * displayManager.width, 2 * displayManager.height);
}

void Render::renderShadowMap(Scene &scene, ShaderProgram &shaderProgram,
                             std::set<LightType> &lightTypes) {
  glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

  for (unsigned int i = 0; i < scene.lights.size(); ++i) {
    Light *light = scene.lights[i];
    if (!lightTypes.count(light->lightType)) {
      continue;
    }
    light->configureShadowMatrices(shaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, light->shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    render(scene, shaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void Render::debugRenderShadowMap(Scene &scene, ShaderProgram &shaderProgram) {
  Light *light = scene.lights[0];
  light->configureShadowMatrices(shaderProgram);
  shaderProgram.uniformSetFloat("near", 1.0f);
  shaderProgram.uniformSetFloat("far", 7.5f);
  render(scene, shaderProgram, false, false, true);
}

void Render::render(Scene &scene, ShaderProgram &shaderProgram, bool withLights,
                    bool withMaterials, bool withShadowMap) {
  // camera
  shaderProgram.uniformSetVec3F("viewPos", scene.camera->getPosition());

  // lights
  if (withLights) {
    configureLights(scene.lights, shaderProgram);
  }

  // models
  for (unsigned int i = 0; i < scene.models.size(); ++i) {
    std::vector<Light *> nullLights{};
    scene.models[i].draw(shaderProgram,
                         withShadowMap ? scene.lights : nullLights,
                         withMaterials);
    std::cout << "render:" << glGetError() << std::endl;
  }
}

void Render::configureLights(std::vector<Light *> &lights,
                             ShaderProgram &shaderProgram) {
  int dirNum = 0, pointNum = 0, spotNum = 0;
  std::string lightIndexStr;
  int depthMapNum = 0;
  for (unsigned int i = 0; i < lights.size(); ++i) {
    Light *light = lights[i];
    light->depthMapIndex = depthMapNum++;
    std::string lightTypeStr = LightTypeToString(light->lightType);
    switch (light->lightType) {
    case LightType::DIRECT:
      lightIndexStr = std::to_string(dirNum++);
      break;
    case LightType::POINT:
      lightIndexStr = std::to_string(pointNum++);
      break;
    case LightType::FLASH:
    case LightType::SPOT:
      lightIndexStr = std::to_string(spotNum++);
      break;
    }
    light->configure(shaderProgram, lightTypeStr, lightIndexStr);
  }
  shaderProgram.uniformSetInt("dirNum", dirNum);
  shaderProgram.uniformSetInt("pointNum", pointNum);
  shaderProgram.uniformSetInt("spotNum", spotNum);
}

void Render::renderSkyBox(Scene &scene, ShaderProgram &shaderProgram) {

  // camera
  glm::mat4 viewTransform = scene.camera->getViewMatrix();
  shaderProgram.uniformSetMat4("view", viewTransform);
  glm::mat4 projectionTransform = scene.camera->getProjectionMatrix(true);
  shaderProgram.uniformSetMat4("projection", projectionTransform);

  glDepthFunc(GL_LEQUAL);
  glBindVertexArray(scene.skyBox->VAO);
  glEnableVertexAttribArray(0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, scene.skyBox->textureID);
  shaderProgram.uniformSetInt("cubemap", 0);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glDepthFunc(GL_LESS);
}

void Render::renderLight(ShaderProgram &shader, glm::vec3 &lightPos,
                         glm::vec3 &diffuse) {
  // cubes
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::mat4(1.0f);
  model = glm::translate(model, lightPos);
  model = glm::scale(model, glm::vec3(0.005f));
  shader.uniformSetMat4("model", model);
  shader.uniformSetVec3F("lightColor", diffuse);
  renderCube();
}

GLuint Render::cubeVAO = 0;
GLuint Render::cubeVBO = 0;

void Render::renderCube() {
  // initialize (if necessary)
  if (cubeVAO == 0) {
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
        1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
        -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // top-left
        // front face
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
        -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
        -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
        -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
        -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
        // right face
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
        1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-right
        1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
        1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
        // bottom face
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
        1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
        1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
        -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
        -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
        1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-right
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
        -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
        -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // bottom-left
    };
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // link vertex attributes
    glBindVertexArray(cubeVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
  // render Cube
  glBindVertexArray(cubeVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

GLuint Render::quadVAO = 0;
GLuint Render::quadVBO = 0;

void Render::deferredRender(ShaderProgram &shader, Scene &scene) {
  std::vector<GLuint> deferredTex = scene.deferredTex;
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, scene.deferredTex[0]);
  shader.uniformSetInt("deferredTex", 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, scene.pingpongColorBuffers[0]);
  shader.uniformSetInt("bloomBlur", 1);

  shader.uniformSetBool("isHdr", true);
  shader.uniformSetBool("isBloom", true);
  shader.uniformSetFloat("exposure", scene.camera->exposure);
  renderQuad();
  std::cout << "renderQuad:" << glGetError() << std::endl;
}

void Render::renderBlur(ShaderProgram &shader, Scene &scene) {
  bool horizontal = true, firstIter = true;
  unsigned int amount = 10;
  for (unsigned int i = 0; i < amount; ++i) {
    glBindFramebuffer(GL_FRAMEBUFFER, scene.pingpongFBO[horizontal]);
    shader.uniformSetBool("horizontal", horizontal);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, firstIter
                                     ? scene.deferredTex[1]
                                     : scene.pingpongColorBuffers[!horizontal]);
    shader.uniformSetInt("image", 0);
    renderQuad();
    horizontal = !horizontal;
    if (firstIter) {
      firstIter = false;
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  std::cout << "renderBlur:" << glGetError() << std::endl;
}

void Render::renderQuad() {
  if (quadVAO == 0) {
    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
  }
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void Render::renderGBuffer(ShaderProgram &shaderProgram, Scene &scene) {
  scene.gBuffer.bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  render(scene, shaderProgram, false, true, false);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  std::cout << "renderGBuffer:" << glGetError() << std::endl;
}

void Render::renderLightPass(ShaderProgram &shaderProgram, Scene &scene) {
  glDisable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shaderProgram.uniformSetVec3F("viewPos", scene.camera->getPosition());
  scene.gBuffer.configure(shaderProgram);
  configureLights(scene.lights, shaderProgram);
  renderQuad();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glEnable(GL_DEPTH_TEST);
  std::cout << "renderLightPass:" << glGetError() << std::endl;
}