#ifndef _BASESINGLETONFACTORY_
#define _BASESINGLETONFACTORY_

#include "baseSingleton.h"
#include <map>
#include <string>

namespace FrameWork_Singletons {
	/// <summary>
	/// eine helper methode um instanzen nach type zu erstellen
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <typeparam name="baseType"></typeparam>
	/// <returns></returns>
	template<typename T, typename baseType>
	baseType* createT() { return new T; };

	/// <summary>
	/// eine Factory
	/// Factory*  Factory::instance = 0;
	/// template<typename T>
	/// struct FactoryRegister : Factory {
	///		FactoryRegister(std::string const& s) {
	///			getMap()->insert(std::make_pair(s, &createT<T, FactoryRegisterType>));
	///		}
	///	};
	/// muss nach der klassen deffinition auch deffiniert werden
	/// </summary>
	/// <typeparam name="baseType">basistype der factory</typeparam>
	template<typename baseType, typename factory>
	class baseFactory : public baseSingleton<factory> {
	protected:
		std::map<std::string, baseType* (*)()> map;

		static std::map<std::string, baseType* (*)()>* getMap() {
			return &(((baseFactory<baseType, factory>*)baseFactory::getInstance())->map);
		}
	public:
		/// <summary>
		/// instancing
		/// </summary>
		baseFactory() {
			map = std::map<std::string, baseType* (*)()>();
		}

		/// <summary>
		/// erstellt eine instance die vorher registriert wurde
		/// über die anweisung 
		/// FactoryRegister<FactoryRegisterType> FactoryRegisterType::reg(typeid(FactoryRegisterType).name());
		/// kann registriert werden
		/// </summary>
		/// <param name="s">der type name der instance</param>
		/// <returns>eine instance pointer die mit new erzeugt wird</returns>
		static baseType* createInstance(std::string const& s) { 
			auto it = ((baseFactory<baseType, factory>*)baseFactory::getInstance())->map.find(s);
			if (it == ((baseFactory<baseType, factory>*)baseFactory::getInstance())->map.end())
				return 0;
			return it->second();
		}
	};
}
#endif