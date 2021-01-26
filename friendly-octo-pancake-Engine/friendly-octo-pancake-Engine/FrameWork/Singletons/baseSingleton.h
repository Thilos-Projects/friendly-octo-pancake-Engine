#ifndef _BASESINGLETON_
#define _BASESINGLETON_

#include <map>
#include <vector>

namespace FrameWork_Singletons {

	/// <summary>
	/// eine abstract warriante eines singletons um alle singletons leichter machen.
	/// T* T::instance = 0; muss nach der klassen deffinition aufgerufen werden
	/// </summary>
	/// <typeparam name="T">der type der classe die ein singleton werden soll</typeparam>
	template<typename T>
	class baseSingleton {
	private:
		/// <summary>
		/// die instance des neuen types
		/// </summary>
		static T* instance;
	public:
		/// <summary>
		/// liefert die instanz des singletons
		/// </summary>
		/// <returns>die instance des singletons</returns>
		static T* getInstance() {
			if (!instance)
				instance = new T();
			return instance;
		}
	};
}

#endif