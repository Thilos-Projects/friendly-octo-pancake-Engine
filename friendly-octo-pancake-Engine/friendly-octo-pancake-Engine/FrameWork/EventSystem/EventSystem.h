#ifndef _EVENTSYSTEM_H_
#define _EVENTSYSTEM_H_

#include "../Singletons/baseSingleton.h"
#include <vector>

namespace FrameWork_EventSystem {

	using namespace FrameWork_Singletons;

	class EventSystem;

	struct Event {
	public:
		bool usable;
		bool destroy;
		virtual void instantFireEvent();
		virtual void updateFireEvent();
	};

	class EventListener {
	public:
		virtual bool onEvent(Event* e) = 0;
	};

	class EventSystem : baseSingleton<EventSystem> {

	};

}
FrameWork_EventSystem::EventSystem* FrameWork_EventSystem::EventSystem::instance = 0;
#endif