#include "pch.h"
#include "includes.h"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include "CCGL.h"

using namespace cocos2d;

#define STRINGIFY(...) #__VA_ARGS__
#pragma comment(linker, "/NODEFAULTLIB:libc.lib")
#define GLEW_STATIC

inline void patch(void* loc, std::vector<std::uint8_t> bytes) {
	auto size = bytes.size();
	DWORD old_prot;
	VirtualProtect(loc, size, PAGE_EXECUTE_READWRITE, &old_prot);
	memcpy(loc, bytes.data(), size);
	VirtualProtect(loc, size, old_prot, &old_prot);
}

float downscale = 1.5; // this is pretty much required because fullhd without downscaling is ~40fps which isnt good
					   // however downscaling isnt too much noticable so 2-1.5 is ok
CCSprite* s;
GLuint m_uniformResolution;
CCLayer* l;

inline bool(__thiscall* init)(CCLayer* self, void* GJGameLevel);
bool __fastcall init_hk(CCLayer* self, int edx, void* GJGameLevel) {
	bool ret = init(self, GJGameLevel);
	if (!ret)
		return ret;

	//https://github.com/matcool/small-gd-mods/blob/main/src/menu-shaders.cpp

	static bool hasPatched = false;
	if (!hasPatched) {
		patch(reinterpret_cast<void*>(gd::base + 0x23b56), { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
		hasPatched = true;
	}	

	auto winSize = CCDirector::sharedDirector()->getWinSize();
	int w = winSize.width;
	int h = winSize.height; // those are in points

	// ADDING NEW LAYER
	auto children = self->getChildren();

	auto node1 = dynamic_cast<CCNode*>(children->objectAtIndex(3));
	auto node2 = dynamic_cast<CCNode*>(children->objectAtIndex(0));
	auto node3 = dynamic_cast<CCNode*>(children->objectAtIndex(4));
	auto node4 = dynamic_cast<CCNode*>(children->objectAtIndex(5));

	l = CCLayer::create();
	self->addChild(l, 5);

	l->addChild(node1);
	l->addChild(node2);
	l->addChild(node3);
	l->addChild(node4);

	l->setScale(l->getScale() / downscale);
	l->setPosition(l->getPosition() - ccp(
		(winSize.width / 2) - (winSize.width / downscale / 2),
		(winSize.height / 2) - (winSize.height / downscale / 2)
	));

	// render

	auto a = CCRenderTexture::create(w / downscale, h / downscale);
	a->getSprite()->getTexture()->setAntiAliasTexParameters();

	a->begin();
	l->visit();
	a->end();
	s = a->getSprite();

	s->setAnchorPoint({ 0,1 });
	s->setPosition({ 0,0 });
	s->setScale(downscale * downscale);
	s->setScaleY(-downscale * downscale);

	self->addChild(s, 5);

	string shaderSource = STRINGIFY(
	uniform vec2 resolution;
	float time = CC_Time.y; // not very accurate but at least works

	uniform sampler2D sprite0;

	void main() {
		vec2 uv = gl_FragCoord / resolution.xy;

		float X = uv.x * 25. + time;
		float Y = uv.y * 25. + time;
		uv.y += cos(X + Y) * 0.005 * cos(Y);
		uv.x += sin(X - Y) * 0.005 * sin(Y);

		gl_FragColor = texture2D(sprite0, uv) * vec4(0.2, 0.4, .9, 1.);
	}
	); // simple underwater shader https://www.shadertoy.com/view/NdKyz1

	auto shader = new CCGLProgram;

	if (!shader->initWithVertexShaderByteArray(STRINGIFY(
		attribute vec4 a_position;

	void main() {
		gl_Position = CC_MVPMatrix * a_position;
	}
	), shaderSource.c_str())) {
		MessageBoxA(NULL, "error", "skill issue", 0);
	}


	shader->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
	shader->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
	shader->link();

	shader->updateUniforms();

	m_uniformResolution = glGetUniformLocation(shader->getProgram(), "resolution");

	auto uniform_tex = glGetUniformLocation(shader->getProgram(), "sprite0");
	glUniform1i(uniform_tex, 0);
	ccGLBindTexture2D(s->getTexture()->getName());

	s->setShaderProgram(shader);

	s->getShaderProgram()->use();
	s->getShaderProgram()->setUniformsForBuiltins();

	auto glv = CCDirector::sharedDirector()->getOpenGLView();
	auto frSize = glv->getFrameSize();
	float wi = frSize.width, he = frSize.height;
	GLfloat vertices[12] = { 0,0, wi,0, wi,he, 0,0, 0,he, wi,he };

	s->getShaderProgram()->setUniformLocationWith2f(m_uniformResolution, wi, he);

	ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position);
	glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glDrawArraysEXT(GL_TRIANGLES, 0, 6);

	shader->release();

	return ret;
}

void(__thiscall* update)(gd::PlayLayer* self, float dt);
void __fastcall update_hk(gd::PlayLayer* self, void*, float dt) {
	update(self, dt);

	auto winSize = CCDirector::sharedDirector()->getWinSize();
	int w = winSize.width;
	int h = winSize.height;

	auto a = CCRenderTexture::create(w / downscale, h / downscale);
	a->getSprite()->getTexture()->setAntiAliasTexParameters();

	a->begin();
	l->visit();
	a->end();

	s->setTexture(a->getSprite()->getTexture());
}

#define CONSOLE 0 // 1 - create a console attached to gd; 0 - dont

DWORD WINAPI my_thread(void* hModule) {
#if CONSOLE
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
#endif // CONSOLE

	MH_Initialize();

	MH_CreateHook(
		(PVOID)(gd::base + 0x1FB780),
		init_hk,
		(LPVOID*)&init
	);

	MH_CreateHook(
		(PVOID)(gd::base + 0x2029C0),
		update_hk,
		(PVOID*)&update
	);

	MH_EnableHook(MH_ALL_HOOKS);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0x1000, my_thread, hModule, 0, 0);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

