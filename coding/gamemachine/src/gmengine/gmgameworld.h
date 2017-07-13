﻿#ifndef __GAMEWORLD_H__
#define __GAMEWORLD_H__
#include "common.h"
#include "foundation/vector.h"
#include "gmphysics/gmphysicsworld.h"
#include "gmdatacore/shader.h"
#include "gmgameobject.h"
#include "foundation/gamemachine.h"

BEGIN_NS

class GMCharacter;
class GMObjectPainter;

GM_PRIVATE_OBJECT(GMGameWorld)
{
	Map<GMGameObjectType, Set<GMGameObject*> > gameObjects;
	bool start;
};

class GMGameWorld : public GMObject
{
	DECLARE_PRIVATE(GMGameWorld)

public:
	GMGameWorld();
	virtual ~GMGameWorld();

public:
	virtual GMPhysicsWorld* physicsWorld() = 0;

public:
	virtual void renderGameWorld();

public:
	void appendObjectAndInit(AUTORELEASE GMGameObject* obj);
	void simulateGameWorld();
	Set<GMGameObject*>& getGameObjects(GMGameObjectType type) { D(d); return d->gameObjects[type]; }
	void addLight(const GMLight& light) { GameMachine::instance().getGraphicEngine()->addLight(light); }

private:
	GMObjectPainter* createPainterForObject(GMGameObject* obj);
};

END_NS
#endif