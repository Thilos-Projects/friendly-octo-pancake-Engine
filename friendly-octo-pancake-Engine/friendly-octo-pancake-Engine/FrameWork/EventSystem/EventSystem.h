#ifndef _EVENTSYSTEM_H_
#define _EVENTSYSTEM_H_

#include "../Singletons/baseSingleton.h"
#include <vector>
#include <map>
#include <queue>

namespace FrameWork_EventSystem {
	using namespace FrameWork_Singletons;

	/// <summary>
	/// ein event
	/// </summary>
	class Event {
	public:
		bool usable;
		static std::string getType() { return ""; }
	};

	/// <summary>
	/// ein event listener
	/// </summary>
	class EventListener {
	private:
		int ID;
	public:
		void setID(int ID) { 
			EventListener::ID = ID;
		};
		virtual bool onEvent(Event* e) = 0;
	};
	
	/// <summary>
	/// das event system
	/// </summary>
	class EventSystem : baseSingleton<EventSystem> {
	private:
		int listenerIDCounter;
		std::map<int, EventListener*> idToListenerMap;
		std::map<std::string, std::vector<EventListener*>> eventToListenerMap;
		std::queue<Event*> eventQue;
	public:

		/// <summary>
		/// initialiesiert ein bischen
		/// </summary>
		EventSystem() {
			listenerIDCounter = 0;
			eventQue = std::queue<Event*>();
			eventToListenerMap = std::map<std::string, std::vector<EventListener*>>();
			idToListenerMap = std::map<int, EventListener*>();
		}

		/// <summary>
		/// registriert ein EventListener
		/// weist diesem eine ID zu
		/// und merkt sich die passenden events
		/// </summary>
		/// <param name="listener">der pointer auf den eventListener. muss bis zum löschen aus dem eventSystem bestehen und gültig bleiben</param>
		/// <param name="eventTypeArray">ein array aus strings die den TypeNamen eines Events enthalten. muss nur während des aufrufs existieren</param>
		/// <param name="amountOfEvents">die anzahl der im array enthaltenden elemente</param>
		void registerListener(EventListener* listener, std::string* eventTypeArray, int amountOfEvents) {
			int ID = listenerIDCounter++;
			listener->setID(ID);
			idToListenerMap.insert(std::pair<int, EventListener*>(ID, listener));
			addEventToListener(listener, eventTypeArray, amountOfEvents);
		}

		/// <summary>
		/// weist einem listener event typen zu
		/// </summary>
		/// <param name="listener">der listener der reagieren soll</param>
		/// <param name="eventTypeArray">ein array aller event typen die angesprochen werden sollen</param>
		/// <param name="amountOfEvents">die anzahl der anzusprechenden typen</param>
		void addEventToListener(EventListener* listener, std::string* eventTypeArray, int amountOfEvents) {
			for (int i = 0; i < amountOfEvents; i++) {
				std::map<std::string, std::vector<EventListener*>>::iterator it = eventToListenerMap.find(eventTypeArray[i]);
				if (it == eventToListenerMap.end())
				{
					eventToListenerMap.insert(std::pair<std::string, std::vector<EventListener*>>(eventTypeArray[i], std::vector<EventListener*>()));
					eventToListenerMap.find(eventTypeArray[i])->second.push_back(listener);
				} else it->second.push_back(listener);
			}
		}

		/// <summary>
		/// weist einem listener event typen zu
		/// </summary>
		/// <param name="listener">die listenerID des listeners der reagieren soll</param>
		/// <param name="eventTypeArray">ein array aller event typen die angesprochen werden sollen</param>
		/// <param name="amountOfEvents">die anzahl der anzusprechenden typen</param>
		void addEventToListener(int listenerID, std::string* eventTypeArray, int amountOfEvents) {
			addEventToListener(idToListenerMap[listenerID], eventTypeArray, amountOfEvents);
		}

		/// <summary>
		/// hängt ein event in die que
		/// </summary>
		/// <param name="e">das anzuhängende event</param>
		void stageEvent(Event* e) {
			eventQue.push(e);
		}

		/// <summary>
		/// sendet ein Event
		/// </summary>
		/// <param name="e">das zu sendene element</param>
		void hitEvent(Event* e) {
			std::vector<EventListener*> temp = eventToListenerMap.find(e->getType())->second;
			for (int i = 0; i < temp.size(); i++) {
				if (temp[i]->onEvent(e) && e->usable)
					break;
			}
			delete(e);
		}

		/// <summary>
		/// sendet alle staged events
		/// </summary>
		void sendStaged() {
			while (!eventQue.empty()) {
				hitEvent(eventQue.front());
				eventQue.pop();
			}
		}
	};
}
FrameWork_EventSystem::EventSystem* FrameWork_EventSystem::EventSystem::instance = 0;
#endif