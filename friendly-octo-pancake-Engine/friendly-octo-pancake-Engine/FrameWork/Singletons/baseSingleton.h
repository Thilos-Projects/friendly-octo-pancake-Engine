#ifndef _BASESINGLETON_
#define _BASESINGLETON_

#include <map>
#include <vector>

namespace FrameWork_Singletons {

	//generic Singleton um jeden type als singleton abbilden zu können
	template<typename T>
	class baseSingleton {
	private:
		//die interne instance des singletons
		static T* instance;
	public:
		//der aufruf um die instatnce zu erriechen
		static T* getInstance() {
			if (!instance)
				instance = new T();
			return instance;
		}
	};
	//	T* T::instance = 0; nach dem erstellen der instance um den singleton zu deffinieren
}

#endif