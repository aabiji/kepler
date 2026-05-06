#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <stb_image.h>

#include "debug.h"
#include "texture.h"

Texture::~Texture() { glDeleteTextures(1, &id); }

void Texture::use(int unit) {
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(obj, id);
}

// If a cubemap texture is being loaded, 6 paths in the following orde: (+x,
// -x, +y, -y, +z, -z), must be supplied. If a 2d texture is being loaded,
// only 1 path must be supplied.
void Texture::init(std::vector<std::string> paths) {
  if (paths.size() != 1 && paths.size() != 6)
    THROW_ERROR("Unkonwn texture usage");

  bool is_cubemap = paths.size() == 6;
  obj = is_cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
  unsigned int bind_obj =
      is_cubemap ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
  unsigned int wrap = is_cubemap ? GL_CLAMP_TO_EDGE : GL_REPEAT;

  glGenTextures(1, &id);
  glBindTexture(obj, id);

  if (!is_cubemap)
    stbi_set_flip_vertically_on_load(true);

  for (size_t i = 0; i < paths.size(); i++) {
    int width = 0, height = 0, channels = 0;
    unsigned char *pixels =
        stbi_load(paths[i].c_str(), &width, &height, &channels, 0);
    if (pixels == nullptr)
      THROW_ERROR("Failed to load {}", paths[i]);

    int internal = channels == 1   ? GL_R16F
                   : channels == 4 ? GL_RGBA16F
                                   : GL_RGB16F;
    int pixel = channels == 1 ? GL_RED : channels == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(bind_obj + i, 0, internal, width, height, 0, pixel,
                 GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
  }

  glTexParameteri(obj, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(obj, GL_TEXTURE_WRAP_T, wrap);
  glTexParameteri(obj, GL_TEXTURE_WRAP_R, wrap);
  glTexParameteri(obj, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(obj, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap(obj);
}

Framebuffer::~Framebuffer() {
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &texture);
}

void Framebuffer::bind(bool use) {
  glBindFramebuffer(GL_FRAMEBUFFER, use ? fbo : 0);
}

unsigned int Framebuffer::read_value(int x, int y) {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
  unsigned int value;
  glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &value);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  return value;
}

void Framebuffer::resize(int width, int height) {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER,
               GL_UNSIGNED_INT, nullptr);
  glViewport(0, 0, width, height);
}

void Framebuffer::init(int width, int height) {
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenTextures(1, &texture);
  resize(width, height);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         texture, 0);

  GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    THROW_ERROR("Incomplete frame buffer");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
