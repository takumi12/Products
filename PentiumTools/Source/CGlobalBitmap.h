#pragma once

#pragma pack(push, 1)
class BITMAP_t
{
public:
	BITMAP_t() {
		uiBitmapIndex = ((GLuint)-1);
		output_width = 0.0;
		output_height = 0.0;
		Components = 4;
	}
	~BITMAP_t() {};
	/*+000*/	GLuint	uiBitmapIndex;
	/*+268*/	float	output_width;
	/*+272*/	float	output_height;
	/*+276*/	char	Components;
};
#pragma pack(pop)

class CGlobalBitmap
{
public:
	CGlobalBitmap();
	~CGlobalBitmap();
	static CGlobalBitmap* Instance();
private:
	float zoom;
	BITMAP_t bitmap;
public:
	void RenderImage();
	bool SaveImage(std::string filename);
	GLuint LoadNodeIcon(int width, int height, BYTE* buffer);
	bool LoadImages(const std::string& filename, GLuint uiFilter = GL_NEAREST, GLuint uiWrapMode = GL_CLAMP);
};

#define GlobalBitmap			(CGlobalBitmap::Instance())
