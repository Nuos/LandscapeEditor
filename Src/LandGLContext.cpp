// --------------------------------------------------------------------
// Created by: Maciej Pryc
// Date: 23.03.2013
// --------------------------------------------------------------------

#include "LandGLContext.h"
#include "LandscapeEditor.h"

#include <sstream>

// --------------------------------------------------------------------
static void CheckGLError()
{
    GLenum errLast = GL_NO_ERROR;

    for ( ;; )
    {
        GLenum err = glGetError();
        if ( err == GL_NO_ERROR )
            return;

        // normally the error is reset by the call to glGetError() but if
        // glGetError() itself returns an error, we risk looping forever here
        // so check that we get a different error than the last time
        if ( err == errLast )
        {
            //wxLogError(wxT("OpenGL error state couldn't be reset."));
            return;
        }

        errLast = err;
        ERR("OpenGL error " << err);
    }
}

// --------------------------------------------------------------------
float LandGLContext::getSecond()
{
	return timeGetTime() / 1000.0f - programStartMoment; // Returns: current time - program start moment = time since program start.
}

// --------------------------------------------------------------------
LandGLContext::LandGLContext(wxGLCanvas *canvas):
wxGLContext(canvas), MouseIntensity(350.0f), CurrentLandscape(0), LandscapeTexture(0), BrushTexture(1), SoilTexture(3), CameraSpeed(0.2f),
OffsetX(0.0001f), OffsetY(0.0001f), ClipmapsAmount(8), VBO(0), IBOs(0), TBOs(0), IBOLengths(0), MovementModifier(10.0f), bNewLandscape(false),
VisibleClipmapStrips(0), CurrentDisplayMode(LANDSCAPE), CurrentMovementMode(ATTACHED_TO_TERRAIN)
{
	programStartMoment = timeGetTime() / 1000.0f;

	VisibleClipmapStrips = new ClipmapStripPair[ClipmapsAmount];

	for (int i = 0; i < ClipmapsAmount; ++i)
		VisibleClipmapStrips[i] = CLIPMAP_STRIP_1;

	IBOs = new GLuint[IBO_MODES_AMOUNT];
	IBOLengths = new int[IBO_MODES_AMOUNT];

	for (int i = 0; i < IBO_MODES_AMOUNT; ++i)
	{
		IBOs[i] = 0;
		IBOLengths[i] = 0;
	}

    for (int i = 0; i < sizeof(Keys); ++i)
        Keys[i] = false;

    SetCurrent(*canvas);
    ((LandGLCanvas*)canvas)->SetOpenGLContext(this);
    
    if (glewInit() == GLEW_OK)
		LOG("GLEW initialized");
	else 
		ERR("Failed to initialize GLEW!");

    CurrentLandscape = new Landscape(9, 1.0f);
    LOG("Initial Landscape created");

    glClearColor(0.6f, 0.85f, 0.9f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(CurrentLandscape->RestartIndex);

    mat4 Scale = scale(1.0f, 1.0f, 1.0f);
    mat4 Rotate = rotate(0.0f, vec3(0.0f, 1.0f, 0.0f));
    mat4 Translate = translate(0.0f, 0.0f, 0.0f);
	Model = Translate * Rotate * Scale;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);

	Projection = perspective(90.0f, ((float)viewport[2] / (float)viewport[3]), 0.1f, 100000.0f);
    LOG("Matrices calculated");

    ResetCamera();
    ResetAllVBOIBO();

    if (LandscapeShad.Initialize("Landscape") == false)
        FatalError("Landscape Shader init failed");
    if (LightningOnlyShad.Initialize("LightningOnly") == false)
        FatalError("Lightning Only Shader init failed");
    if (HeightShad.Initialize("Height") == false)
        FatalError("Height Shader init failed");
    if (WireframeShad.Initialize("Wireframe") == false)
        FatalError("Wireframe Shader init failed");
	if (ClipmapWireframeShad.Initialize("ClipmapWireframe") == false)
        FatalError("Clipmap Wireframe Shader init failed");
	if (ClipmapLandscapeShad.Initialize("ClipmapLandscape") == false)
        FatalError("Clipmap Landscape Shader init failed");
    
	SetShadersInitialUniforms();

	switch (CurrentDisplayMode)
	{
	case LANDSCAPE:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		ClipmapLandscapeShad.Use();
		break;
	case WIREFRAME:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		ClipmapWireframeShad.Use();
		break;
	}

    LOG("Loading textures...");

    // ----------------------------- Landscape Texture --------------------------------
    glActiveTexture(GL_TEXTURE0);
    //if (TextureManager::Inst()->LoadTexture("Content/Textures/grass.tga", LandscapeTexture))
	if (TextureManager::Inst()->LoadTexture("Content/Textures/test_diffuse.tga", LandscapeTexture))
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		WARN("Can't load grass.tga texture!");
	}

    // ----------------------------- Brush Texture --------------------------------
    glActiveTexture(GL_TEXTURE1);
    if (TextureManager::Inst()->LoadTexture("Content/Textures/Brush2a.png", BrushTexture, GL_RGBA, GL_RGBA))
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		WARN("Can't load Brush2a.png texture!");
	}


    // ----------------------------- Soil Texture --------------------------------
    glActiveTexture(GL_TEXTURE3);
    if (TextureManager::Inst()->LoadTexture("Content/Textures/smallrocks.tga", SoilTexture))
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		WARN("Can't load smallrocks.tga texture!");
	}

    LOG("Textures loaded");

    CurrentBrush.SetPosition(vec3(0.0f));

	// ----------------------------- Texture Buffer Objects (TBOs) --------------------------------

	TBOs = new GLuint[ClipmapsAmount];
	glGenBuffers(ClipmapsAmount, TBOs);

	int ClipmapScale = 1;

	for (int i = 0; i < ClipmapsAmount; ++i)
	{
		InitTBO(TBOs[i], ClipmapScale);
		ClipmapScale *= 2;
	}

    CheckGLError();

    ((LandGLCanvas*)canvas)->SetOpenGLContextInitialized(true);

	SetVSync(false);
	CONF("==== Initialization completed! ====");
}

// --------------------------------------------------------------------
LandGLContext::~LandGLContext(void)
{
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(IBO_MODES_AMOUNT, IBOs);
	glDeleteBuffers(ClipmapsAmount, TBOs);

	glDeleteTextures(1, &LandscapeTexture);
    glDeleteTextures(1, &SoilTexture);
    glDeleteTextures(1, &BrushTexture);

	delete[] IBOs;
	delete[] IBOLengths;
}

// --------------------------------------------------------------------
void LandGLContext::SetShadersInitialUniforms()
{
	WireframeShad.Use();
    WireframeShad.SetBrushTextureSampler(1);
    WireframeShad.SetTBOSampler(2);
    WireframeShad.SetBrushPosition(vec2(0.0f, 0.0f));
    WireframeShad.SetBrushScale(1.0f);
    //WireframeShad.SetLandscapeSizeX(CurrentLandscape->GetClipmapVBOWidth() + 1);
    WireframeShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    WireframeShad.SetWireframeColor(vec3(0.0f, 0.0f, 0.0f));
    WireframeShad.SetBrushColor(vec3(1.0f, 1.0f, 1.0f));
	WireframeShad.SetTestOffsetX(0.0f);
	WireframeShad.SetTestOffsetY(0.0f);
	WireframeShad.SetgWorld(mat4(0.0f));
	WireframeShad.SetClipmapPartOffset(vec2(0.0f, 0.0f));

	ClipmapWireframeShad.Use();
	ClipmapWireframeShad.SetBrushTextureSampler(1);
	ClipmapWireframeShad.SetTBOSampler(2);
    ClipmapWireframeShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
    ClipmapWireframeShad.SetBrushScale(CurrentBrush.GetRadius() * 2.0f);
    ClipmapWireframeShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    ClipmapWireframeShad.SetBrushColor(vec3(1.0f, 1.0f, 1.0f));
	ClipmapWireframeShad.SetWireframeColor(vec3(0.6f, 0.0f, 0.0f));
	ClipmapWireframeShad.SetCameraOffsetX(0.0f);
	ClipmapWireframeShad.SetCameraOffsetY(0.0f);
	ClipmapWireframeShad.SetgWorld(mat4(0.0f));
	ClipmapWireframeShad.SetClipmapScale(1.0f);
	ClipmapWireframeShad.SetClipmapWidth(CurrentLandscape->GetTBOSize());

	LandscapeShad.Use();
    LandscapeShad.SetBrushTextureSampler(1);
    LandscapeShad.SetTBOSampler(2);
    LandscapeShad.SetBrushPosition(vec2(0.0f, 0.0f));
    LandscapeShad.SetBrushScale(1.0f);
    //LandscapeShad.SetLandscapeSizeX(CurrentLandscape->GetClipmapVBOWidth() + 1);
    LandscapeShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    LandscapeShad.SetWireframeColor(vec3(0.0f, 0.0f, 0.0f));
    LandscapeShad.SetBrushColor(vec3(1.0f, 1.0f, 1.0f));
	LandscapeShad.SetTestOffsetX(0.0f);
	LandscapeShad.SetTestOffsetY(0.0f);
	LandscapeShad.SetgWorld(mat4(0.0f));
	LandscapeShad.SetClipmapPartOffset(vec2(0.0f, 0.0f));
	LandscapeShad.SetTextureSampler(0);

	ClipmapLandscapeShad.Use();
    ClipmapLandscapeShad.SetBrushTextureSampler(1);
    ClipmapLandscapeShad.SetTBOSampler(2);
    ClipmapLandscapeShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
    ClipmapLandscapeShad.SetBrushScale(CurrentBrush.GetRadius() * 2.0f);
    ClipmapLandscapeShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
	ClipmapLandscapeShad.SetCameraOffsetX(0.0f);
	ClipmapLandscapeShad.SetCameraOffsetY(0.0f);
	ClipmapLandscapeShad.SetgWorld(mat4(0.0f));
	ClipmapLandscapeShad.SetClipmapScale(1.0f);
	ClipmapLandscapeShad.SetClipmapWidth(CurrentLandscape->GetTBOSize());
	ClipmapLandscapeShad.SetTextureSampler(0);
}

// --------------------------------------------------------------------
void LandGLContext::InitTBO(GLuint TBOID, int ClipmapScale)
{
	int TBOSize = CurrentLandscape->GetTBOSize();

	glActiveTexture(GL_TEXTURE2);
	float *Data = new float[TBOSize * TBOSize];
	float *Heightmap = CurrentLandscape->GetHeightmap();
	int StartIndexX = CurrentLandscape->GetStartIndexX();
	int StartIndexY = CurrentLandscape->GetStartIndexY();
	unsigned int DataSize = CurrentLandscape->GetHeightDataSize();

	for (int x = 0; x < TBOSize; ++x)
	{
		for (int y = 0; y < TBOSize; ++y)
		{
			int IndexX = int(mod(float(StartIndexX + ClipmapScale + ((TBOSize + 1) / 2) * (ClipmapScale - 1) + (x - TBOSize) * ClipmapScale), float(DataSize)));
			int IndexY = int(mod(float(StartIndexY + ClipmapScale + ((TBOSize + 1) / 2) * (ClipmapScale - 1) + (y - TBOSize) * ClipmapScale), float(DataSize)));

			Data[y * TBOSize + x] = Heightmap[IndexY * DataSize + IndexX];
		}
	}
			
	glBindBuffer(GL_TEXTURE_BUFFER, TBOID);
	glBufferData(GL_TEXTURE_BUFFER, TBOSize * TBOSize * sizeof(float), Data, GL_STATIC_DRAW);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, TBOID);

	delete[] Data;
}

// --------------------------------------------------------------------
void LandGLContext::DrawScene()
{	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 MVP = Projection * View * Model;

    CheckGLError();

    glActiveTexture(GL_TEXTURE2);
    glEnableVertexAttribArray(0);

	switch (CurrentDisplayMode)
	{
	case LANDSCAPE:	ClipmapLandscapeShad.SetgWorld(MVP);	break;
	case WIREFRAME:	ClipmapWireframeShad.SetgWorld(MVP);	break;
	}

	float Scale = 1.0f;
	for (int lvl = 0; lvl < ClipmapsAmount; lvl++, Scale *= 2.0f)
	{
		switch (CurrentDisplayMode)
		{
		case LANDSCAPE: ClipmapLandscapeShad.SetClipmapScale(Scale); break;
		case WIREFRAME: 
			ClipmapWireframeShad.SetClipmapScale(Scale); 
			if (lvl % 2 == 0)
				ClipmapWireframeShad.SetWireframeColor(vec3(0.0f, 0.0f, 0.0f));
			else
				ClipmapWireframeShad.SetWireframeColor(vec3(0.6f, 0.0f, 0.0f));
			break;
		}

		switch (VisibleClipmapStrips[lvl])
		{
		case CLIPMAP_STRIP_1:
			RenderLandscapeModule(lvl == 0 ? IBO_CENTER_1 : IBO_CLIPMAP_1, TBOs[lvl]);
			break;
		case CLIPMAP_STRIP_2:
			RenderLandscapeModule(lvl == 0 ? IBO_CENTER_2 : IBO_CLIPMAP_2, TBOs[lvl]);
			break;
		case CLIPMAP_STRIP_3:
			RenderLandscapeModule(lvl == 0 ? IBO_CENTER_3 : IBO_CLIPMAP_3, TBOs[lvl]);
			break;
		case CLIPMAP_STRIP_4:
			RenderLandscapeModule(lvl == 0 ? IBO_CENTER_4 : IBO_CLIPMAP_4, TBOs[lvl]);
			break;
		}
	}

    glDisableVertexAttribArray(0);

    glFlush();
    CheckGLError();
}

// --------------------------------------------------------------------
void LandGLContext::RenderLandscapeModule(const ClipmapIBOMode IBOMode, GLuint TBOID)
{
	glBindBuffer(GL_TEXTURE_BUFFER, TBOID);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, TBOID);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOs[IBOMode]);

	glDrawElements(GL_TRIANGLE_STRIP, IBOLengths[IBOMode], GL_UNSIGNED_INT, 0);
}

// --------------------------------------------------------------------
void LandGLContext::SetVSync(bool sync)
{	
	// Function pointer for the wgl extention function we need to enable/disable
	// vsync
	typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALPROC)( int );
	PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = 0;

	const char *extensions = (char*)glGetString( GL_EXTENSIONS );

	if( strstr( extensions, "WGL_EXT_swap_control" ) == 0 )
	{
		return;
	}
	else
	{
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress( "wglSwapIntervalEXT" );

		if( wglSwapIntervalEXT )
			wglSwapIntervalEXT(sync);
	}
}

// --------------------------------------------------------------------
void LandGLContext::OnKey(bool bKeyIsDown, wxKeyEvent& event)
{
	static float MovementSpeed = 0.0f;

    switch (event.GetKeyCode())
    {
        case WXK_UP:
            Keys[4] = bKeyIsDown;
			
			if (bKeyIsDown)
			{
				OffsetY += 2.0f;
				OffsetX += 2.0f;
				ClipmapWireframeShad.SetCameraOffsetX(OffsetX);
				ClipmapWireframeShad.SetCameraOffsetY(OffsetY);
				UpdateTBO();
			}
            break;

        case WXK_DOWN:
            Keys[5] = bKeyIsDown;

			if (bKeyIsDown)
			{
				OffsetY += 2.0f;
				OffsetX -= 2.0f;
				ClipmapWireframeShad.SetCameraOffsetX(OffsetX);
				ClipmapWireframeShad.SetCameraOffsetY(OffsetY);
				UpdateTBO();
			}
            break;

        case WXK_RIGHT:
            Keys[6] = bKeyIsDown;

			if (bKeyIsDown)
			{
				OffsetY -= 2.0f;
				OffsetX += 2.0f;
				ClipmapWireframeShad.SetCameraOffsetX(OffsetX);
				ClipmapWireframeShad.SetCameraOffsetY(OffsetY);
				UpdateTBO();
			}
            break;

		case WXK_LEFT:
            Keys[7] = bKeyIsDown;

			if (bKeyIsDown)
			{
				OffsetY -= 2.0f;
				OffsetX -= 2.0f;
				ClipmapWireframeShad.SetCameraOffsetX(OffsetX);
				ClipmapWireframeShad.SetCameraOffsetY(OffsetY);
				UpdateTBO();
			}
            break;

		case (16 * wxMOD_SHIFT + WXK_CONTROL_W):
            Keys[0] = bKeyIsDown;
            break;


        case (16 * wxMOD_SHIFT + WXK_CONTROL_S):
            Keys[1] = bKeyIsDown;
            break;


        case (16 * wxMOD_SHIFT + WXK_CONTROL_D):
            Keys[2] = bKeyIsDown;
            break;

        case (16 * wxMOD_SHIFT + WXK_CONTROL_A):
            Keys[3] = bKeyIsDown;
            break;
       
        case WXK_F1:
            if (bKeyIsDown)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                CurrentDisplayMode = LANDSCAPE;
				ClipmapLandscapeShad.Use();
				ClipmapLandscapeShad.SetCameraOffsetX(OffsetX);
				ClipmapLandscapeShad.SetCameraOffsetY(OffsetY);
            }
            break;
        case WXK_F2:
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                CurrentDisplayMode = WIREFRAME;
				ClipmapWireframeShad.Use();
				ClipmapWireframeShad.SetCameraOffsetX(OffsetX);
				ClipmapWireframeShad.SetCameraOffsetY(OffsetY);
            }
            break;
        case WXK_SPACE:
            Keys[8] = bKeyIsDown;
            break;
    }
}

// --------------------------------------------------------------------
void LandGLContext::OnResize(wxSize NewSize)
{
    glViewport(0, 0, NewSize.x, NewSize.y);
	Projection = perspective(90.0f, ( (float)NewSize.x / (float)NewSize.y), 0.1f, 100000.0f);
}

// --------------------------------------------------------------------
void LandGLContext::UpdateBrushPosition()
{
    GLfloat winZ;
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glReadPixels(MouseX, viewport[3] - MouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
    vec3 screenPos = vec3(MouseX, viewport[3] - MouseY, winZ);
    vec3 worldPos = unProject(screenPos, View, Projection, vec4(0.0f, 0.0f, viewport[2], viewport[3]));

    CurrentBrush.SetPosition(worldPos);

	switch (CurrentDisplayMode)
	{
	case LANDSCAPE: ClipmapLandscapeShad.SetBrushPosition(CurrentBrush.GetRenderPosition()); break;
	case WIREFRAME:	ClipmapWireframeShad.SetBrushPosition(CurrentBrush.GetRenderPosition()); break;
	}
}

// --------------------------------------------------------------------
void LandGLContext::OnMouse(wxMouseEvent& event)
{
    static float StartX = 0.0f, StartY = 0.0f;

	MouseX = event.GetX();
	MouseY = event.GetY();

    if (event.RightIsDown())
    {
        if (event.RightDown())
        {
            LandscapeEditor::Inst()->Frame->SetCursor(wxCursor(wxCURSOR_BLANK));

            StartX = event.GetX();
            StartY = event.GetY();
        }

        CameraHorizontalAngle -= (event.GetX() - StartX) / MouseIntensity;
        CameraVerticalAngle -= (event.GetY() - StartY) / MouseIntensity;

        LandscapeEditor::Inst()->Frame->WarpPointer(StartX, StartY);

        vec3 Direction(cos(CameraVerticalAngle) * sin(CameraHorizontalAngle), sin(CameraVerticalAngle), cos(CameraVerticalAngle) * cos(CameraHorizontalAngle));
        vec3 Right = vec3(sin(CameraHorizontalAngle - 3.14f/2.0f), 0, cos(CameraHorizontalAngle - 3.14f/2.0f));
        vec3 Up = cross(Right, Direction);
        View = lookAt(CameraPosition, CameraPosition + Direction, Up);
    }
    else
    {
        UpdateBrushPosition();
    }

    //if (event.GetWheelRotation() != 0)
    //{
    //    (event.GetWheelRotation() < 0) ? (CurrentBrush.ModifyRadius(1.1f)) : (CurrentBrush.ModifyRadius(0.9f));

    //    if ((*CurrentShader) == LandscapeShad)
    //    {
    //        LandscapeShad.SetBrushScale(CurrentBrush.GetRadius() * 2.0f);
    //        LandscapeShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
    //    }
    //    else if ((*CurrentShader) == LightningOnlyShad)
    //    {
    //        LightningOnlyShad.SetBrushScale(CurrentBrush.GetRadius() * 2.0f);
    //        LightningOnlyShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
    //    }
    //    else if ((*CurrentShader) == HeightShad)
    //    {
    //        HeightShad.SetBrushScale(CurrentBrush.GetRadius() * 2.0f);
    //        HeightShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
    //    }
    //    else if ((*CurrentShader) == WireframeShad)
    //    {
    //        WireframeShad.SetBrushScale(CurrentBrush.GetRadius() * 2.0f);
    //        WireframeShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
    //    }
    //}

    if (event.RightUp())
        LandscapeEditor::Inst()->Frame->SetCursor(wxNullCursor);

    if (event.LeftIsDown()) Keys[9] = true;
    else  Keys[9] = false;
}

// --------------------------------------------------------------------
void LandGLContext::ManageInput()
{
	static float MovementSpeed = 0.1f;

	float CurrentMovementModifier = (Keys[8]) ? (MovementModifier) : (1.0f);

    if (Keys[0] || Keys[1] || Keys[2] || Keys[3])
    {
        vec3 Direction(cos(CameraVerticalAngle) * sin(CameraHorizontalAngle), sin(CameraVerticalAngle), cos(CameraVerticalAngle) * cos(CameraHorizontalAngle));
        vec3 Right = vec3(sin(CameraHorizontalAngle - 3.14f/2.0f), 0, cos(CameraHorizontalAngle - 3.14f/2.0f));
        vec3 Up = cross(Right, Direction);

		if (Keys[0])
		{
			OffsetX += (CurrentMovementModifier * MovementSpeed * sin(CameraHorizontalAngle)) / CurrentLandscape->GetOffset();
			OffsetY += (CurrentMovementModifier * MovementSpeed * cos(CameraHorizontalAngle)) / CurrentLandscape->GetOffset();
			CameraPosition.y += (Direction * (CameraSpeed * ((Keys[8]) ? (MovementModifier) : (1.0f)))).y;
		}
		if (Keys[1])
		{
			OffsetX -= (CurrentMovementModifier * MovementSpeed * sin(CameraHorizontalAngle)) / CurrentLandscape->GetOffset();
			OffsetY -= (CurrentMovementModifier * MovementSpeed * cos(CameraHorizontalAngle)) / CurrentLandscape->GetOffset();
			CameraPosition.y -= (Direction * (CameraSpeed * ((Keys[8]) ? (MovementModifier) : (1.0f)))).y;
		}
		if (Keys[2])
		{
			OffsetX += (CurrentMovementModifier * MovementSpeed * sin(CameraHorizontalAngle - 3.14f/2.0f)) / CurrentLandscape->GetOffset();
			OffsetY += (CurrentMovementModifier * MovementSpeed * cos(CameraHorizontalAngle - 3.14f/2.0f)) / CurrentLandscape->GetOffset();
			CameraPosition.y += (Right * (CameraSpeed * ((Keys[8]) ? (MovementModifier) : (1.0f)))).y;
		}
		if (Keys[3])
		{
			OffsetX -= (CurrentMovementModifier * MovementSpeed * sin(CameraHorizontalAngle - 3.14f/2.0f)) / CurrentLandscape->GetOffset();
			OffsetY -= (CurrentMovementModifier * MovementSpeed * cos(CameraHorizontalAngle - 3.14f/2.0f)) / CurrentLandscape->GetOffset();
			CameraPosition.y -= (Right * (CameraSpeed * ((Keys[8]) ? (MovementModifier) : (1.0f)))).y;
		}

		switch (CurrentDisplayMode)
		{
		case LANDSCAPE:
			ClipmapLandscapeShad.SetCameraOffsetX(OffsetX);
			ClipmapLandscapeShad.SetCameraOffsetY(OffsetY);
			ClipmapLandscapeShad.SetBrushPosition(CurrentBrush.GetRenderPosition());
			break;
		case WIREFRAME:
			ClipmapWireframeShad.SetCameraOffsetX(OffsetX);
			ClipmapWireframeShad.SetCameraOffsetY(OffsetY);
			break;
		}

		UpdateBrushPosition();
		UpdateTBO();

		View = lookAt(CameraPosition, CameraPosition + Direction, Up);
    }
}

// --------------------------------------------------------------------
void LandGLContext::CreateNewLandscape(int Size)
{
    if (CurrentLandscape != 0)
        delete CurrentLandscape;

    CurrentLandscape = new Landscape(Size, 1.0f);

    ResetAllVBOIBO();
    ResetCamera();

    //if ((*CurrentShader) == LandscapeShad)
    //{
    //    LandscapeShad.SetLandscapeSizeX(CurrentLandscape->GetTBOSize());
    //    LandscapeShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    //}
    //else if ((*CurrentShader) == LightningOnlyShad)
    //{
    //    LightningOnlyShad.SetLandscapeSizeX(CurrentLandscape->GetTBOSize());
    //    LightningOnlyShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    //}
    //else if ((*CurrentShader) == HeightShad)
    //{
    //    HeightShad.SetLandscapeSizeX(CurrentLandscape->GetTBOSize());
    //    HeightShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    //    HeightShad.SetMaxHeight(CurrentLandscape->GetTBOSize() * CurrentLandscape->GetOffset() * 0.15f);
    //}
    //else if ((*CurrentShader) == WireframeShad)
    //{
    //    WireframeShad.SetLandscapeSizeX(CurrentLandscape->GetTBOSize());
    //    WireframeShad.SetLandscapeVertexOffset(CurrentLandscape->GetOffset());
    //}

    CheckGLError();
}

// --------------------------------------------------------------------
void LandGLContext::SaveLandscape(const char* FilePath)
{
    //if (CurrentLandscape != 0)
    //    CurrentLandscape->SaveToFile(FilePath);

	//LOG("Saving...");

	//LandscapeEditor::FileWrite(FilePath, DATA, DataSize * DataSize);

	//LOG("Completed!");
}

// --------------------------------------------------------------------
void LandGLContext::OpenFromFile(const char* FilePath)
{
	LOG("Opening...");

	//delete[] DATA;

	//DATA = new float[DataSize * DataSize];
	//unsigned int DataByteSize;

	//DATA = (float*)LandscapeEditor::FileRead(FilePath, DataByteSize);
	//DataSize = sqrt((float)DataByteSize / 4.0);
	//StartIndexY = StartIndexX = DataSize / 2.0f;

	//int ClipmapScale = 1;
	//for (int i = 0; i < ClipmapsAmount; ++i)
	//{
	//	InitTBO(TBOs[i], ClipmapScale);
	//	ClipmapScale *= 2;
	//}

	//ResetCamera();
	//SetShadersInitialUniforms();

	//for (int i = 0; i < ClipmapsAmount; ++i)
	//	VisibleClipmapStrips[i] = CLIPMAP_STRIP_1;

	//bNewLandscape = true;
	//LOG("Completed!");

 //   CheckGLError();
}

// --------------------------------------------------------------------
void LandGLContext::FatalError(char* text)
{
    ERR("Fatal error ocured!");
    ERR(text);
    system("pause");
    exit(1);
}

// --------------------------------------------------------------------
void LandGLContext::ResetCamera()
{
	OffsetX = 0.0001f;
	OffsetY = 0.0001f;

	CameraPosition = vec3(0.0f, 100.0f, 0.0f);
	CameraVerticalAngle = -1.57f;
    CameraHorizontalAngle = 0.0f;

    vec3 Direction(cos(CameraVerticalAngle) * sin(CameraHorizontalAngle), sin(CameraVerticalAngle), cos(CameraVerticalAngle) * cos(CameraHorizontalAngle));
    vec3 Right = vec3(sin(CameraHorizontalAngle - 3.14f/2.0f), 0, cos(CameraHorizontalAngle - 3.14f/2.0f));
    vec3 Up = cross(Right, Direction);
    View = lookAt(CameraPosition, CameraPosition + Direction, Up);
}

// --------------------------------------------------------------------
void LandGLContext::ResetVBO(GLuint &BufferID, float *NewData, int DataSize)
{
	if (BufferID != 0)
        glDeleteBuffers(1, &BufferID);

 	glGenBuffers(1, &BufferID);
	glBindBuffer(GL_ARRAY_BUFFER, BufferID);
	glBufferData(GL_ARRAY_BUFFER, DataSize * sizeof(float), NewData, GL_STATIC_DRAW);
}

// --------------------------------------------------------------------
void LandGLContext::ResetIBO(GLuint &BufferID, unsigned int *NewData, int DataSize)
{
	if (BufferID != 0)
		glDeleteBuffers(1, &BufferID);

    glGenBuffers(1, &BufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, DataSize * sizeof(unsigned int), NewData, GL_STATIC_DRAW);
}

// --------------------------------------------------------------------
void LandGLContext::ResetAllVBOIBO()
{
	float *ClipmapVBOData;
	int ClipmapVBOSize;
	unsigned int **ClipmapIBOsData = new unsigned int*[IBO_MODES_AMOUNT];

	ClipmapVBOData = CurrentLandscape->GetClipmapVBOData(ClipmapVBOSize);
	ResetVBO(VBO, ClipmapVBOData, ClipmapVBOSize);

	for (int i = 0; i < IBO_MODES_AMOUNT; ++i)
	{
		ClipmapIBOsData[i] = CurrentLandscape->GetClipmapIBOData((ClipmapIBOMode)i, IBOLengths[i]);
		ResetIBO(IBOs[i], ClipmapIBOsData[i], IBOLengths[i]);
	}

	delete [] ClipmapIBOsData;
}

// --------------------------------------------------------------------
void LandGLContext::UpdateTBO()
{
	static float *ClipmapLastUpdateOffsetX = 0;
	static float *ClipmapLastUpdateOffsetY = 0;

	if (bNewLandscape)
	{
		delete [] ClipmapLastUpdateOffsetX;
		delete [] ClipmapLastUpdateOffsetY;

		ClipmapLastUpdateOffsetX = 0;
		ClipmapLastUpdateOffsetY = 0;

		bNewLandscape = false;
	}

	if (ClipmapLastUpdateOffsetX == 0 && ClipmapLastUpdateOffsetY == 0)
	{
		ClipmapLastUpdateOffsetX = new float[ClipmapsAmount];
		ClipmapLastUpdateOffsetY = new float[ClipmapsAmount];

		float scale11 = 1.0f;

		for (int i = 0; i < ClipmapsAmount; ++i)
		{
			ClipmapLastUpdateOffsetX[i] = scale11;
			ClipmapLastUpdateOffsetY[i] = scale11;
			scale11 *= 2.0f;
		}
	}

	float *BufferData32 = NULL;
	int TBOSize = CurrentLandscape->GetTBOSize();
	float *Heightmap = CurrentLandscape->GetHeightmap();
	unsigned int DataSize = CurrentLandscape->GetHeightDataSize();
	int StartIndexX = CurrentLandscape->GetStartIndexX();
	int StartIndexY = CurrentLandscape->GetStartIndexY();
	int ClipmapScale = 1;

	for (int lvl = 0; lvl < ClipmapsAmount; ++lvl)
	{
		float fDiffX = OffsetX - ClipmapLastUpdateOffsetX[lvl];
		float fDiffY = OffsetY - ClipmapLastUpdateOffsetY[lvl];

		int SignX = sign(fDiffX);
		int SignY = sign(fDiffY);
		
		int	DiffX = floor((abs(fDiffX) + ClipmapScale) / (2.0f * ClipmapScale)) * SignX * 2.0f;
		int	DiffY = floor((abs(fDiffY) + ClipmapScale) / (2.0f * ClipmapScale)) * SignY * 2.0f;

		// Update VisibleClipmapStrip value
		if (mod(OffsetX, 2.0f * ClipmapScale) < ClipmapScale)
		{
			if (mod(OffsetY, 2.0f * ClipmapScale) < ClipmapScale)
				VisibleClipmapStrips[lvl] = CLIPMAP_STRIP_1;
			else
				VisibleClipmapStrips[lvl] = CLIPMAP_STRIP_2;
		}
		else
		{
			if (mod(OffsetY, 2.0f * ClipmapScale) < ClipmapScale)
				VisibleClipmapStrips[lvl] = CLIPMAP_STRIP_3;
			else
				VisibleClipmapStrips[lvl] = CLIPMAP_STRIP_4;
		}

		if (DiffX != 0 || DiffY != 0)
		{
			glActiveTexture(GL_TEXTURE2);	

			glBindBuffer(GL_TEXTURE_BUFFER, TBOs[lvl]);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, TBOs[lvl]);
			BufferData32 = (float*)glMapBuffer(GL_TEXTURE_BUFFER, GL_WRITE_ONLY);

			for (int j = 0; j < abs(DiffX); j++)
			{
				for (int i = max(0, DiffY); i < TBOSize + min(0, DiffY); i++)
				{
					int xTBO = int(mod(ClipmapLastUpdateOffsetX[lvl] / ClipmapScale - ((SignX > 0) ? (2.0f) : (1.0f)) + SignX * (j + 1), float(TBOSize)));
					int yTBO = int(mod(ClipmapLastUpdateOffsetY[lvl] / ClipmapScale + i - 1.0f, float(TBOSize)));
					int x = int(mod(StartIndexX + ((TBOSize + 3) / 2) * (ClipmapScale - 1) + ClipmapLastUpdateOffsetX[lvl] - ClipmapScale + SignX * (j * ClipmapScale + 1) - ((SignX < 0) ? ((TBOSize + 1) * ClipmapScale - 2.0f) : (0.0f)), float(DataSize)));
					int y = int(mod(StartIndexY - ((TBOSize - 3) / 2) * (ClipmapScale - 1) - (TBOSize - 1.0f) + ClipmapLastUpdateOffsetY[lvl] - ClipmapScale + i * ClipmapScale, float(DataSize)));

					BufferData32[yTBO * TBOSize + xTBO] = Heightmap[y * DataSize + x];
				}
			}
		
			for (int j = 0; j < abs(DiffY); j++)
			{
				for (int i = 0; i < TBOSize; i++)
				{
					int xTBO = int(mod(ClipmapLastUpdateOffsetX[lvl] / ClipmapScale + i - 1.0f + DiffX, float(TBOSize)));
					int yTBO = int(mod(ClipmapLastUpdateOffsetY[lvl] / ClipmapScale - ((SignY > 0) ? (2.0f) : (1.0f)) + SignY * (j + 1), float(TBOSize)));
					int x = int(mod(StartIndexX - ((TBOSize - 3) / 2) * (ClipmapScale - 1) - (TBOSize - 1.0f) + ClipmapLastUpdateOffsetX[lvl] - ClipmapScale + (i + DiffX) * ClipmapScale, float(DataSize)));
					int y = int(mod(StartIndexY + ((TBOSize + 3) / 2) * (ClipmapScale - 1) + ClipmapLastUpdateOffsetY[lvl] - ClipmapScale + SignY * (j * ClipmapScale + 1) - ((SignY < 0) ? (TBOSize * ClipmapScale + (ClipmapScale - 2.0f)) : (0.0f)), float(DataSize)));

					BufferData32[yTBO * TBOSize + xTBO] = Heightmap[y * DataSize + x];
				}
			}

			glUnmapBuffer(GL_TEXTURE_BUFFER);	
			
			ClipmapLastUpdateOffsetX[lvl] += min(abs(DiffX), TBOSize) * SignX * ClipmapScale;
			ClipmapLastUpdateOffsetY[lvl] += min(abs(DiffY), TBOSize) * SignY * ClipmapScale;
		}
		else
		{
			break;
		}

		ClipmapScale *= 2;
	}
}