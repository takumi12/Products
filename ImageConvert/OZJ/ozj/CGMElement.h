#pragma once
#include <setjmp.h>

#pragma pack(push, 1)
typedef struct
{
/*+000*/	GLuint	uiBitmapIndex;
/*+268*/	float	output_width;
/*+272*/	float	output_height;
/*+276*/	char	Components;
} BITMAP_t;
#pragma pack(pop)

struct my_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr* my_error_ptr;