#ifndef _ENTRYPOINT_
#define _ENTRYPOINT_

#include "Singletons/baseSingleton.h"
#include "Singletons/baseSingletonFactory.h"

namespace FrameWork
{
	using namespace FrameWork_Singletons;
	/// <summary>
	/// vererbt alle wichtigen funktionen für behaviours
	/// ist keine state maschine updates, starts, awakes, und delayedStarts haben eine nicht spezifizierte abfolge mit nicht paraleler datenstruktur
	/// behaviourFactoryRegister<T> T::reg(typeid(T).name());
	/// zum registrieren
	/// </summary>
	class mainBehaviour {
	public :
		/// <summary>
		/// beim erstellen des behaviours
		/// sollte auf keine anderen behaviours zugreifen und hat keine spefizierte reienfolge
		/// </summary>
		virtual void Awake() = 0;
		/// <summary>
		/// wird vor dem ersten generellen Update ausgeführt
		/// ist die klasse da noch nicht erstellt wird Start nicht aufgerufen
		/// </summary>
		virtual void Start() = 0;

		/// <summary>
		/// vor dem ersten Update das die classe kennt ausgeführt
		/// </summary>
		virtual void delayedStart() = 0;

		/// <summary>
		/// wird jeden update ausgeführt
		/// </summary>
		virtual void Update() = 0;

		/// <summary>
		/// nach dem letzten update oder vor dem zerstören durch thanos
		/// </summary>
		virtual void onDestroy() = 0;
	};

	/// <summary>
	/// ist die Factory für alle behaviours
	/// </summary>
	class behaviourFactory : private baseFactory<mainBehaviour> {
	private:
		std::vector<mainBehaviour*> allExisting;
		std::vector<mainBehaviour*> toDestroy;

		/// <summary>
		/// erstellt einen vector
		/// </summary>
		behaviourFactory() {
			allExisting = std::vector<mainBehaviour*>();
			toDestroy = std::vector<mainBehaviour*>();
		}
	public:

		/// <summary>
		/// erstellt eine instatnc eines types
		/// </summary>
		/// <typeparam name="T"> der zu erstllende type</typeparam>
		/// <returns>ein pointer auf die bestellte instance</returns>
		template<typename T>
		static T* instantiate() {
			T* temp = (T*)createInstance(typeid(T).name);
			((behaviourFactory*)getInstance()).allExisting.push_back(temp);
			return temp;
		}

		/// <summary>
		/// zerstört eine instanz einer mainBehaviour mit dem nächsten aufruf von hasTime
		/// </summary>
		/// <param name="toDestroy">die zu zerstörende instanz</param>
		static void destroy(mainBehaviour* toDestroy) {
			behaviourFactory* me = ((behaviourFactory*)getInstance());
			me->toDestroy.push_back(toDestroy);
		}

		/// <summary>
		/// gibt einen verweis auf alle vorhandenen mainBehaviours raus die bekannt sind
		/// </summary>
		/// <returns>ein pointer auf ein vector von mainBehaviour pointern</returns>
		static std::vector<mainBehaviour*>* getAll() {
			return &((behaviourFactory*)getInstance())->allExisting;
		}

		/// <summary>
		/// solte nach dem aufruf von destroy aufgerufen werden sobalt nicht mehr über alle exestierenden itteriert wird
		/// </summary>
		static void hasTime() {
			behaviourFactory* me = ((behaviourFactory*)getInstance());
			for (int j = 0; j < me->toDestroy.size(); j++) {
				int id = -1;
				for (int i = 0; i < me->allExisting.size(); i++)
					if (me->allExisting[i] == me->toDestroy[j])
						id = i;
				if (id != -1)
					me->allExisting.erase(me->allExisting.begin() + id);
				me->toDestroy[j]->onDestroy();
				delete(me->toDestroy[j]);
			}
			me->toDestroy.clear();
		}
	};
	
	/// <summary>
	/// ein helper um behaviours zu erstellen
	/// </summary>
	/// <typeparam name="T">der zu erstellende type</typeparam>
	/// <returns></returns>
	template<typename T>
	mainBehaviour* createbehaviour() {
		mainBehaviour* temp = new T();
		temp->Awake();
		return temp;
	};

	/// <summary>
	/// ist ein helper für das erstellen von behaviours in der factory
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	struct behaviourFactoryRegister : behaviourFactory {
		behaviourFactoryRegister(std::string const& s, bool instantMake) {
			getMap()->insert(std::make_pair(s, &createbehaviour<T>));
			if (instantMake)
				behaviourFactory::createInstance(s);
		}
	};

	class mainClass : baseSingleton<mainClass> {
	public:
		bool endGame;

		mainClass() {
			endGame = false;
		}

		void runner() {
			std::vector<mainBehaviour*>* it = behaviourFactory::getAll();
			for (int i = 0; i < it->size(); i++)
				it->at(i)->Start();
			behaviourFactory::hasTime();

			while (!endGame)
			{
				it = behaviourFactory::getAll();
				for (int i = 0; i < it->size(); i++)
					it->at(i)->Update();
				behaviourFactory::hasTime();
			}
			it = behaviourFactory::getAll();
			for (int i = 0; i < it->size(); i++)
				it->at(i)->onDestroy();
			behaviourFactory::hasTime();
		}
	};

}
FrameWork::behaviourFactory* FrameWork::behaviourFactory::instance = 0;
FrameWork::mainClass* FrameWork::mainClass::instance = 0;

#endif