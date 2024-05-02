#include "framework.h"
#include "CGMLibrayMng.h"
#include "CGMFileSystem.h"
#include "CGlobalBitmap.h"

CGlobalBitmap::CGlobalBitmap()
{
	zoom = 1.0;
}

CGlobalBitmap::~CGlobalBitmap()
{
}

CGlobalBitmap* CGlobalBitmap::Instance()
{
	static CGlobalBitmap tc; return &tc;
}

void CGlobalBitmap::RenderImage()
{
	if ((&bitmap)->uiBitmapIndex != ((GLuint)-1) && GMFileSystem->IsNodoSelect() == true)
	{
		ImGui::BeginChild("Image View", ImVec2(0, -150), ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);

		char ImageName[MAX_PATH];
		float width = (&bitmap)->output_width;
		float hight = (&bitmap)->output_height;

		sprintf_s(ImageName, "%s [W: %.1f][H: %.1f][zoom: %d%%]", GMFileSystem->GetFileName(), width, hight, (int)(zoom * 100));

		ImGui::Text(ImageName);
		ImGui::Separator();

		ImVec2 uv_min = ImVec2(0.0f, 0.0f);

		ImVec2 uv_max = ImVec2(1.0f, 1.0f);

		ImTextureID TextureId = (ImTextureID)((&bitmap)->uiBitmapIndex);

		//ImVec4* colors = (&ImGui::GetStyle())->Colors;

		ImGui::Image(TextureId, ImVec2((float)(width * zoom), (float)(hight * zoom)), uv_min, uv_max);

		ImGui::EndChild();

		if (ImGui::BeginPopupContextItem("MenuImagen"))
		{
			if (ImGui::MenuItem("Zoom(+)"))
			{
				if (zoom < 16.0) zoom *= 2.f;
			}
			if (ImGui::MenuItem("Zoom(-)"))
			{
				if (zoom > 0.25) zoom /= 2.f;
			}
			if (ImGui::MenuItem("ResetZoom"))
			{
				zoom = 1.0;
			}
			if (ImGui::MenuItem("Save Image"))
			{
				GMFileSystem->SaveImage();
			}
			ImGui::EndPopup();
		}
	}
}

bool CGlobalBitmap::SaveImage(std::string filename)
{
	char __ext[_MAX_EXT] = { 0, };
	_splitpath(filename.c_str(), NULL, NULL, NULL, __ext);

	return GMLibrayMng->SaveImagen(__ext, filename, true);
}

GLuint CGlobalBitmap::LoadNodeIcon(int width, int height, BYTE* data)
{
	GLuint textureID;

	glGenTextures(1, &(textureID));
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	//glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureID;
}

bool CGlobalBitmap::LoadImages(const std::string& filename, GLuint uiFilter, GLuint uiWrapMode)
{
	if (bitmap.uiBitmapIndex != ((GLuint)-1))
	{
		zoom = 1.0;
		glDeleteTextures(1, &(bitmap.uiBitmapIndex));
		bitmap.uiBitmapIndex = ((GLuint)-1);
	}

	char __ext[_MAX_EXT] = { 0, };
	_splitpath(filename.c_str(), NULL, NULL, NULL, __ext);

	return GMLibrayMng->OpenImagen(__ext, filename, &bitmap, uiFilter, uiWrapMode);
}