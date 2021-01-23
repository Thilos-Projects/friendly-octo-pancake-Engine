#ifndef _BASESINGLETONFACTORY_
#define _BASESINGLETONFACTORY_

#include <map>
#include <string>

namespace FrameWork_Singletons {

	//helper methode um type vom baseType zu erstellen
	template<typename T, typename baseType>
	baseType* createT() { return new T; };

	//eine factory um belibige objecte eines abstrakten types zu erstellen
	template<typename baseType>
	struct baseFactory {
	public:
		//ist kein basic basic storrage dar funktionen pointer gespeichert werden müssen
		typedef std::map<std::string, baseType* (*)()> map_type;

		static baseType* createInstance(std::string const& s) {
			auto it = getMap()->find(s);
			if (it == getMap()->end())
				return 0;
			return it->second();
		}

	protected:
		static map_type* getMap() {
			if (!map) { map = new map_type; }
			return map;
		}

	private:
		static map_type* map;
	};
}

#endif