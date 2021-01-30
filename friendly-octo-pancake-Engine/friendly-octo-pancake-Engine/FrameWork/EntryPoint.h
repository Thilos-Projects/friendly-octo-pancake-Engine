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
	private:

		typedef void(*FuncPtr)(mainBehaviour*);
		FuncPtr callback;
		void set_callback(FuncPtr fp) { callback = fp; }

		void firstUpdate() {
			Start();
			Update();
			set_callback([](mainBehaviour* me) { me->Update(); });
		}

	public :

		void CallUpdate() { callback(this); }

		mainBehaviour() {
			set_callback([](mainBehaviour* me) { me->firstUpdate(); });
		}

		

		/// <summary>
		/// beim erstellen des behaviours
		/// sollte auf keine anderen behaviours zugreifen und hat keine spefizierte reienfolge
		/// </summary>
		virtual void Awake() = 0;

		/// <summary>
		/// vor dem ersten Update das die classe kennt ausgeführt
		/// </summary>
		virtual void Start() = 0;

		/// <summary>
		/// wird jeden update ausgeführt
		/// </summary>
		virtual void Update() = 0;

		/// <summary>
		/// nach dem letzten update oder vor dem zerstören durch thanos
		/// </summary>
		virtual void onDestroy() = 0;
	};

	/*
	* hier ein minimal aufbau eines main behaviours
	class T : public FrameWork::mainBehaviour {
	private:
		static FrameWork::behaviourFactoryRegister<T> reg;
	public:

		void Awake() {
		}
		void Start() {
		}
		void delayedStart() {
		}
		void Update() {
		}
		void onDestroy() {
		}
	};
	FrameWork::behaviourFactoryRegister<T> T::reg(typeid(T).name(), true); 
	//das true sagd das eine !!nicht frei zugreifbare!! instanze vor dem instanzieren des hauptcodes erstellt werden soll dies ist z.b. für den renderer vorgesehen auch der scenen loader wird so geladen
	*/

	/// <summary>
	/// ist die Factory für alle behaviours
	/// </summary>
	class behaviourFactory : public baseFactory<mainBehaviour,behaviourFactory> {
	private:
		std::vector<mainBehaviour*> allExisting;
		std::vector<mainBehaviour*> toDestroy;

	public:

		/// <summary>
		/// erstellt einen vector
		/// </summary>
		behaviourFactory() {
			allExisting = std::vector<mainBehaviour*>();
			toDestroy = std::vector<mainBehaviour*>();
		}

		/// <summary>
		/// erstellt eine instatnc eines types
		/// </summary>
		/// <typeparam name="T"> der zu erstllende type</typeparam>
		/// <returns>ein pointer auf die bestellte instance</returns>
		template<typename T>
		static T* instantiate() {
			T* temp = (T*)createInstance(typeid(T).name());
			if (temp != NULL)
				((behaviourFactory*)getInstance())->allExisting.push_back(temp);
			return temp;
		}

		/// <summary>
		/// erstellt eine instatnc eines types
		/// </summary>
		/// <typeparam name="T"> der zu erstllende type</typeparam>
		/// <returns>ein pointer auf die bestellte instance</returns>
		static mainBehaviour* instantiate(std::string typeName) {
			mainBehaviour* temp = (mainBehaviour*)createInstance(typeName);
			if (temp != NULL)
				((behaviourFactory*)getInstance())->allExisting.push_back(temp);
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

		/// <summary>
		/// clears everything
		/// !destroys everything!
		/// </summary>
		static void clear() {
			behaviourFactory* me = ((behaviourFactory*)getInstance());
			for (int i = 0; i < me->allExisting.size(); i++)
				me->toDestroy.push_back(me->allExisting[i]);
			hasTime();
		}
	};
	
	/// <summary>
	/// ein helper um behaviours zu erstellen
	/// </summary>
	/// <typeparam name="T">der zu erstellende type</typeparam>
	/// <returns></returns>
	template<typename T>
	mainBehaviour* createbehaviour() {
		mainBehaviour* temp = (mainBehaviour*)new T();
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
				behaviourFactory::instantiate<T>();
		}
	};

	class mainClass : private baseSingleton<mainClass> {
	private:
		bool endGame;
	public:

		mainClass() {
			endGame = false;
		}
		/// <summary>
		/// endGame
		/// </summary>
		static void playThanosTheme() {
			mainClass* mC = ((mainClass*)getInstance());
			mC->endGame = true;
		}

		/// <summary>
		/// startet die engine
		/// </summary>
		static void run() {
			mainClass* mC = ((mainClass*)getInstance());

			std::vector<mainBehaviour*>* it;

			while (!mC->endGame)
			{
				it = behaviourFactory::getAll();
				for (int i = 0; i < it->size(); i++)
					it->at(i)->CallUpdate();
				behaviourFactory::hasTime();
			}
			behaviourFactory::clear();
		}
	};
}
FrameWork::behaviourFactory* FrameWork::behaviourFactory::instance = 0;
FrameWork::mainClass* FrameWork::mainClass::instance = 0;

#endif