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

/** Shader for drawing terrain outlines - clipmap compatible version */
class ClipmapWireframeShader : public Shader
{
public:
    /// Uniform setters
	void SetBrushTextureSampler(int Value) {SetUniform("BrushTextureSampler", Value);};
	void SetTBOSampler(int Value) {SetUniform("TBOSampler", Value);};
	void SetgWorld(mat4 Value) {SetUniform("gWorld", Value);};
	void SetBrushPosition(vec2 Value) {SetUniform("BrushPosition", Value);};
	void SetBrushScale(float Value) {SetUniform("BrushScale", Value);};
	void SetClipmapWidth(int Value) {SetUniform("ClipmapWidth", Value);};
	void SetLandscapeVertexOffset(float Value) {SetUniform("LandscapeVertexOffset", Value);};
	void SetWireframeColor(vec3 Value) {SetUniform("WireframeColor", Value);};
	void SetBrushColor(vec3 Value) {SetUniform("BrushColor", Value);};
	void SetCameraOffsetX(float Value) {SetUniform("CameraOffsetX", Value);};
	void SetCameraOffsetY(float Value) {SetUniform("CameraOffsetY", Value);};
	void SetClipmapScale(int Value) {SetUniform("ClipmapScale", Value);};

    /// Standard constructor
	ClipmapWireframeShader()
	{
		Uniforms.insert(std::make_pair<std::string, GLuint>("BrushTextureSampler", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("TBOSampler", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("gWorld", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("BrushPosition", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("BrushScale", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("ClipmapWidth", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("LandscapeVertexOffset", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("WireframeColor", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("BrushColor", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("CameraOffsetX", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("CameraOffsetY", 0));
		Uniforms.insert(std::make_pair<std::string, GLuint>("ClipmapScale", 0));
	}
};