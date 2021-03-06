// --------------------------------------------------------------------
// Created by: Maciej Pryc
// Date: 23.03.2013
// --------------------------------------------------------------------
#pragma once

#include "Brush.h"

enum ClipmapIBOMode		{IBO_CENTER_1,
						IBO_CENTER_2,
						IBO_CENTER_3,
						IBO_CENTER_4,
						IBO_CLIPMAP_1,
						IBO_CLIPMAP_2,
						IBO_CLIPMAP_3,
						IBO_CLIPMAP_4,
						IBO_MODES_AMOUNT};

enum ClipmapStripPair	{CLIPMAP_STRIP_1, 
						CLIPMAP_STRIP_2, 
						CLIPMAP_STRIP_3, 
						CLIPMAP_STRIP_4, 
						CLIPMAP_STRIP_PAIRS_AMOUNT};

/** the main terrain class */
class Landscape
{
public:
    /// Index used for primitive restart when drawing
    const unsigned int RestartIndex;

protected:
    /// Distance between two adjacent vertices
    float Offset;

    /// HeightData
    float *HeightData;
	unsigned int HeightDataSize;
	int StartIndexX;
	int StartIndexY;

	/// VBO Data
	float *ClipmapVBOData;
	unsigned int VBOSize;
	unsigned int ClipmapVBOWidth;

	/// IBO Data
	unsigned int **ClipmapIBOsData;
	unsigned int *IBOSize;

	/// TBO Data
	unsigned int TBOSize;

public:
    /// Standard constructors and destructor
    Landscape(int ClipmapRimWidth, float VerticesInterval);
    Landscape(const char* FilePath);
    ~Landscape();

    /// Save heightmap to file, return true if succeeded
    bool SaveToFile(const char* FilePath);

    /// Change landscape height data
    void UpdateHeightmap(Brush &AffectingBrush);

    /// Getters
	float * GetClipmapVBOData(int &outDataAmount);
	unsigned int * GetClipmapIBOData(ClipmapIBOMode Mode, int &outDataAmount);
	unsigned int GetTBOSize() {return TBOSize;};
	float * GetHeightmap() {return HeightData;};
	unsigned int GetHeightDataSize() {return HeightDataSize;};
    float GetOffset() {return Offset;};
	int GetStartIndexX() {return StartIndexX;};
	int GetStartIndexY() {return StartIndexY;};

	// Not working assignment operator
	Landscape & operator= (Landscape & other) {return other;};
   
protected: 
	void CreateVBO();
	void CreateIBO(ClipmapIBOMode Mode);
	unsigned int * ConstructNiceIBOData(unsigned int Width, bool bOffsetX, bool bOffsetY, unsigned int CenterHoleWidth, unsigned int &DataSize);
};