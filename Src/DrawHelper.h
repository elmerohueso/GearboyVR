#include <OVR_FileSys.h>
#include <OVR_Input.h>
#include <VrApi_Input.h>
#include <VrApi_Types.h>
#include <fstream>
#include <iostream>
#include <map>

#include <VRMenu.h>
#include <dirent.h>
#include <algorithm>
#include "GuiSys.h"
#include "OVR_Locale.h"
#include "OvrApp.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#ifndef ANDROID_DRAWHELPER_H
#define ANDROID_DRAWHELPER_H

#endif  // ANDROID_DRAWHELPER_H

void InitDrawHelper(GLfloat menuWidth, GLfloat menuHeight);

void DrawRectangle(GLfloat posX, GLfloat posY, GLfloat width, GLfloat height, ovrVector4f color);

void DrawTexture(GLuint textureId, GLfloat posX, GLfloat posY, GLfloat width, GLfloat height,
                 ovrVector4f color);