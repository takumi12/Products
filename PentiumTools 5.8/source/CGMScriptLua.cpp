#include "stdafx.h"
#include "CGMScriptLua.h"
#include "./Utilities/Log/muConsoleDebug.h"

CGMLuaBase::CGMLuaBase()
{
	lua.open_libraries(sol::lib::base);
}

CGMLuaBase::~CGMLuaBase()
{
}

bool CGMLuaBase::runtime_link(std::vector<BYTE> DataBytes)
{
	try {

		std::string scriptContent = std::string(DataBytes.begin(), DataBytes.end());

		this->runtime_link_model();
		this->runtime_link_object();
		this->runtime_link_character();
		this->runtime_link_funtion();

		lua.script(scriptContent);
		return true;
	}
	catch (const sol::error& e) {
		g_ConsoleDebug->Write(5, "Error runtime_link: %s", e.what());
		//std::cerr << "Error loading script from buffer: " << e.what() << std::endl;
	}
	return false;
}

bool CGMLuaBase::RenderLinkAction(PART_t* f, OBJECT* pObj)
{
	this->pass_to_lua("pObj", pObj);

	sol::function runtime_function_unique = lua["RenderLinkAction"];

	if (runtime_function_unique.valid()) {
		//try {
		//	runtime_function_unique(f);
		//	return true;
		//}
		//catch (const sol::error& e) {
		//	//g_ConsoleDebug->Write(5, "Error calling RenderLinkAction: %s", e.what());
		//}
		runtime_function_unique(f);
		return true;
	}
	else
	{
		//g_ConsoleDebug->Write(5, "Function RenderLinkAction not found in Lua script");
	}

	return false;
}

bool CGMLuaBase::RenderPartObjectBody(BMD* pModel, OBJECT* pObj, float Alpha, int RenderType)
{
	this->pass_to_lua("pObj", pObj);
	this->pass_to_lua("pModel", pModel);
	this->pass_to_lua_value("Texture", 0);
	this->pass_to_lua_value("Alpha", Alpha);
	this->pass_to_lua_value("RenderType", RenderType);
	this->pass_to_lua_table("pBodyLight", pModel->BodyLight, 3);
	//this->pass_to_lua_value("RenderType", RenderType);

	sol::function runtime_function_unique = lua["RenderPartObjectBody"];

	if (runtime_function_unique.valid()) {
		try {
			runtime_function_unique();
			return true;
		}
		catch (const sol::error& e) {
			//g_ConsoleDebug->Write(5, "Error calling RenderPartObjectBody: %s", e.what());
		}
	}
	else {
		//g_ConsoleDebug->Write(5, "Function RenderPartObjectBody not found in Lua script");
	}

	return false;
}

bool CGMLuaBase::RenderEffectObjectBody(BMD* pModel, OBJECT* pObj, float Alpha, int RenderType, int Texture)
{
	this->pass_to_lua("pObj", pObj);
	this->pass_to_lua("pModel", pModel);
	this->pass_to_lua_value("Alpha", Alpha);
	this->pass_to_lua_value("Texture", Texture);
	this->pass_to_lua_value("RenderType", RenderType);
	this->pass_to_lua_table("pBodyLight", pModel->BodyLight, 3);

	sol::function runtime_function_unique = lua["RenderEffectObjectBody"];

	if (runtime_function_unique.valid())
	{
		pModel->BeginRender(Alpha);

		if (!pModel->LightEnable)
		{
			if (pObj->Alpha >= 0.99f)
				glColor3fv(pModel->BodyLight);
			else
				glColor4f(pModel->BodyLight[0], pModel->BodyLight[1], pModel->BodyLight[2], pObj->Alpha);
		}

		runtime_function_unique();

		pModel->EndRender();

		return true;
	}
	else {
		//g_ConsoleDebug->Write(5, "Function RenderEffectObjectBody not found in Lua script");
	}

	return false;
}

void CGMLuaBase::runtime_link_model()
{
	lua.new_usertype<BMD>("BMD",
		sol::constructors<BMD()>(),
		"NumMeshs", &BMD::NumMeshs,
		"BodyLight", &BMD::BodyLight,
		"StreamMesh", &BMD::StreamMesh,
		"HideSkin", &BMD::HideSkin,
		"EndRender", &BMD::EndRender,
		"BeginRender", &BMD::BeginRender,
		"GetBodyLight", &BMD::GetBodyLight,
		"RenderMesh", &BMD::RenderMesh,
		"RenderBody", &BMD::RenderBody
		);
}

void CGMLuaBase::runtime_link_object()
{
	lua.new_usertype<OBJECT>("OBJECT",
		sol::constructors<OBJECT()>(),
		"Alpha", &OBJECT::Alpha,
		"BlendMesh", &OBJECT::BlendMesh,
		"CurrentAction", &OBJECT::CurrentAction,
		"BlendMeshLight", &OBJECT::BlendMeshLight,
		"BlendMeshTexCoordU", &OBJECT::BlendMeshTexCoordU,
		"BlendMeshTexCoordV", &OBJECT::BlendMeshTexCoordV,
		"AnimationFrame", &OBJECT::AnimationFrame
		);
}

void CGMLuaBase::runtime_link_character()
{
	lua.new_usertype<PART_t>("PART_t",
		sol::constructors<PART_t()>(),
		"Type", &PART_t::Type,
		"CurrentAction", &PART_t::CurrentAction,
		"PriorAction", &PART_t::PriorAction,
		"AnimationFrame", &PART_t::AnimationFrame,
		"PriorAnimationFrame", &PART_t::PriorAnimationFrame,
		"PlaySpeed", &PART_t::PlaySpeed,
		"m_byNumCloth", &PART_t::m_byNumCloth
		);
}

void CGMLuaBase::runtime_link_funtion()
{
	lua.set_function("RenderAlphaG", &Render22);
	lua.set_function("Vector", &to_funtion__Vector);
	lua.set_function("cast_int", &to_funtion__cast_int);
	lua.set_function("glColor4f", &to_funtion__glColor4f);
	lua.set_function("glColor3fv", &to_funtion__glColor3fv);
	lua.set_function("VectorCopy", &to_funtion__VectorCopy);
	lua.set_function("timeGetTime", &to_funtion__timeGetTime);
}

int to_funtion__cast_int(float number)
{
	return static_cast<int>(number);
}

int to_funtion__timeGetTime()
{
	return timeGetTime();
}

void to_funtion__glColor3fv(float* v)
{
	glColor3fv(v);
}

void to_funtion__glColor4f(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}

void to_funtion__Vector(float a, float b, float c, float* v)
{
	if (v)
	{
		v[0] = a;
		v[1] = b;
		v[2] = c;
	}
}

void to_funtion__VectorCopy(sol::table lua_table, float* b)
{
	if (b)
	{
		b[0] = lua_table.get<float>(1);
		b[1] = lua_table.get<float>(2);
		b[2] = lua_table.get<float>(3);
	}
}

void to_funtion__RenderLevel(int GroupMesh, float Red, float Blue, float Green)
{
	//BMD* b = lua[uniqueId];
}
