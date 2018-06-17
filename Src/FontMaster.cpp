#include <OVR_FileSys.h>
#include <VrApi_Types.h>
#include <OVR_Input.h>
#include <VrApi_Input.h>
#include <iostream>
#include <fstream>
#include <map>

#include <dirent.h>
#include <VRMenu.h>
#include "OvrApp.h"
#include "GuiSys.h"
#include "OVR_Locale.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "FontMaster.h"

using namespace OVR;

FT_Library ft;
GLuint VAO, VBO;
GLuint fontTexture = 0;

GlProgram glProgram;

GLfloat vertices[6 * 100][4];

static const char VERTEX_SHADER[] =
        "#version 330 core\n"
                "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
                //"layout (location = 1) in vec2 texture;  // <vec2 pos, vec2 tex>\n"

                "out vec2 TexCoords;\n"

                "uniform mat4 projection;\n"

                "void main()\n"
                "{\n"
                "	gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
                "	TexCoords = vertex.zw;\n"
                "}\n";

static const char FRAGMENT_SHADER[] =
        "#version 330 core\n"
                "in vec2 TexCoords;\n"

                "out vec4 color;\n"

                "uniform sampler2D text;\n"
                "uniform vec3 textColor;\n"

                "void main()\n"
                "{\n"
                //"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).a);\n"
                "	color = vec4(textColor, 1.0) * texture(text, TexCoords).a;\n"
                "}\n";

void InitFontMaster(GLfloat menuWidth, GLfloat menuHeight) {
    if (FT_Init_FreeType(&ft))
        LOG("ERROR::FREETYPE: Could not init FreeType Library");

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4 * 100, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glProgram = GlProgram::Build(VERTEX_SHADER, FRAGMENT_SHADER, NULL, 0);
    glUseProgram(glProgram.Program);

    glm::mat4 projection = glm::ortho(0.0f, menuWidth, 0.0f, menuHeight);
    glUniformMatrix4fv(glGetUniformLocation(glProgram.Program, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));

    LOG("FontMaster Initialized");
}

void LoadFont(RenderFont *font, char *filePath, uint fontSize) {
    LOG("start loading font");

    font->FontSize = fontSize;

    FT_Face face;

    if (FT_New_Face(ft, filePath, 0, &face))
        LOG("ERROR::FREETYPE: Failed to load font");

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // create a texture for the characters
    int textureWidth = 10 * fontSize;
    int textureHeight = 10 * fontSize;
    glGenTextures(1, &font->textureID);
    glBindTexture(GL_TEXTURE_2D, font->textureID);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_ALPHA,
            textureWidth,
            textureHeight,
            0,
            GL_ALPHA,
            GL_UNSIGNED_BYTE,
            NULL);

    int posX = 1;
    int posY = 1;

    for (GLubyte c = 0; c < 128; c++) {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }

        LOG("Character: %i, %i, %f", posX, posY,
            ((float) (posX + face->glyph->bitmap.width) / textureWidth));

        // go to the next line
        if (posX + face->glyph->bitmap.width > textureWidth) {
            posX = 0;
            posY += fontSize + 2;
        }

        /*
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_ALPHA,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_ALPHA,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );
        */

        glTexSubImage2D(GL_TEXTURE_2D, 0, posX, posY,
                        face->glyph->bitmap.width,
                        face->glyph->bitmap.rows,
                        GL_ALPHA, GL_UNSIGNED_BYTE,
                        face->glyph->bitmap.buffer);

        // Now store character for later use
        Character character = {
                glm::fvec4((float) posX / textureWidth, (float) posY / textureHeight,
                           (float) (posX + face->glyph->bitmap.width) / textureWidth,
                           (float) (posY + face->glyph->bitmap.rows) / textureHeight),
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                ((GLuint) face->glyph->advance.x >> 6)
        };
        font->Characters.insert(std::pair<GLchar, Character>(c, character));

        posX += face->glyph->bitmap.width + 2;
    }

    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);

    LOG("finished loading fonts");
}

void CloseFontLoader() {
    FT_Done_FreeType(ft);
}

int GetWidth(RenderFont font, std::string text){
    int width = 0;

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = font.Characters[*c];
        width += (ch.Advance);
    }

    return width;
}

void StartFontRendering() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_DST_ALPHA, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    glUseProgram(glProgram.Program);

    fontTexture = 0;
}

void
RenderText(RenderFont font, std::string text, GLfloat x, GLfloat y, GLfloat scale, Vector3f color) {

    // Iterate through all characters
    std::string::const_iterator c;
    int index = 0;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = font.Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y + font.FontSize - (ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character

        GLfloat charVertices[6][4] = {
                {xpos,     ypos + h, ch.Position.x, ch.Position.w},
                {xpos,     ypos,     ch.Position.x, ch.Position.y},
                {xpos + w, ypos,     ch.Position.z, ch.Position.y},

                {xpos,     ypos + h, ch.Position.x, ch.Position.w},
                {xpos + w, ypos,     ch.Position.z, ch.Position.y},
                {xpos + w, ypos + h, ch.Position.z, ch.Position.w}
        };

/*
        vertices[6 * index + 0] = {xpos, ypos + h, ch.Position.x, ch.Position.w};
        vertices[6 * index + 1] = {xpos, ypos, ch.Position.x, ch.Position.y};
        vertices[6 * index + 2] = {xpos + w, ypos, ch.Position.z, ch.Position.y};
        vertices[6 * index + 3] = {xpos, ypos + h, ch.Position.x, ch.Position.w};
        vertices[6 * index + 4] = {xpos + w, ypos, ch.Position.z, ch.Position.y};
        vertices[6 * index + 5] = {xpos + w, ypos + h, ch.Position.z, ch.Position.w}; */

        memcpy(&vertices[6 * index], &charVertices, sizeof(charVertices));

        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
        index++;
    }

    // Activate corresponding render state
    glUniform3f(glGetUniformLocation(glProgram.Program, "textColor"), color.x, color.y, color.z);

    if (fontTexture == 0 || fontTexture != font.textureID) {
        fontTexture = font.textureID;
        glBindTexture(GL_TEXTURE_2D, font.textureID);
    }

    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6 * text.length());
}

void CloseTextRenderer() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}