// --------------------------------------------------------------------
// Created by: Maciej Pryc
// Date:
// --------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "Shader.h"

using namespace glm;

/** Shader which cause terrain to be render without landscape texture, only lightning */
class LightningOnlyShader : public Shader
{
protected:
    /// Uniforms
    GLuint uMVP, uBrushPosition, uBrushScale, uBrushTexture, uTBO, uLandscapeSizeX, uLandscapeVertexOffset, uAmbientStrength, uDiffuseStrength;

public:
    /// Standard constructors
    LightningOnlyShader();

    /// Uniform setters
    void SetBrushTexture(int TextureUnit) {glUniform1i(uBrushTexture, TextureUnit);};
    void SetTBO(int TBO) {glUniform1i(uTBO, TBO);};
    void SetMVP(mat4 MVP) {glUniformMatrix4fv(uMVP, 1, GL_FALSE, &MVP[0][0]);};
    void SetBrushPosition(vec2 BrushPosition) {glUniform2fv(uBrushPosition, 1, &BrushPosition[0]);};
    void SetBrushScale(float BrushScale) {glUniform1f(uBrushScale, BrushScale);};
    void SetLandscapeSizeX(float LandscapeSizeX) {glUniform1f(uLandscapeSizeX, LandscapeSizeX);};
    void SetLandscapeVertexOffset(float LandscapeVertexOffset) {glUniform1f(uLandscapeVertexOffset, LandscapeVertexOffset);};
    void SetAmbientStrength(float AmbientStrength) {glUniform1f(uAmbientStrength, AmbientStrength);};
    void SetDiffuseStrength(float DiffuseStrength) {glUniform1f(uDiffuseStrength, DiffuseStrength);};

protected:
    /// Initialize uniform variables, return false when failure, true on success
    virtual bool InitializeUniforms();
};