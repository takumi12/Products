//////////////////////////////////////////////////////////////////////////
//  GOBoid.h
//////////////////////////////////////////////////////////////////////////
#ifndef __GOBOID_H__
#define __GOBOID_H__

#include "zzzObject.h"

typedef struct
{
	int nType;
	int RenderIndex;
	int MovementType;
	float RenderScale;
}PET_GOBOID;

void CreateGoboid(int ItemIndex, vec3_t Position, OBJECT* Owner, int SubType = 0, int LinkBone = 0);

void CreateBug(int Type, vec3_t Position, OBJECT* Owner, int SubType = 0, int LinkBone = 0);
void DeleteBug(OBJECT* Owner);



void CreateMuun(int Type, vec3_t Position, OBJECT* Owner, int SubType = 0, int LinkBone = 0);
void DeleteMuun(OBJECT* Owner);

void MoveBugs();
void RenderBugs();
bool IsBug(ITEM* pItem);
bool isGoBoidItem(int ItemIndex);


bool CreateBugSub(int Type, vec3_t Position, OBJECT* Owner, OBJECT* o, int SubType = 0, int LinkBone = 0);
bool MoveBug(OBJECT* o, bool bForceRender = false);
bool RenderBug(OBJECT* o, bool bForceRender = false);
void RenderDarkHorseSkill(OBJECT* o, BMD* b);
void RenderSkillEarthQuake(CHARACTER* c, OBJECT* o, BMD* b, int iMaxSkill = 30);



void MoveBoids(void);
void RenderBoids(bool bAfterCharacter = false);
void DeleteBoids(void);

void MoveFishs(void);
void RenderFishs(void);

void GoBoidREG(int nType, int Type, int Movement, float scale);

extern std::map<int, PET_GOBOID> goBoidList;
#endif// __GOBOID_H__