#ifndef _BASESINGLETONSTORRAGE_
#define _BASESINGLETONSTORRAGE_

#include "baseSingleton.h"

namespace FrameWork_Singletons {

	//ein einfacher singleton speicher basierend auf einer map
	//I ist sortier Type
	//T ist speicher type
	//P ist type des Speichers
	template<typename I, typename T, typename P>
	class baseMapStorrage : public baseSingleton<P> {
	protected:
		//die map von objecten T
		std::map<I, T*> objects;

		//constructer für das erstellen der map
		baseMapStorrage() {
			objects = std::map<I, T*>();
		};
	public:
		//add object to Map
		void addOnId(I id, T* obj) {
			objects.insert(std::pair<int, T*>(id, obj));
		}

		//get object from map
		T* getById(I ID) {
			auto it = objects.find(ID);
			if (it == objects.end())
				return NULL;
			return it->second;
		}

		//remove object from map
		static void rempoveByID(I ID) {
			//toDo
		}
	};

	//ein einfacher singleton speicher basierend auf einer map
	//T ist speicher type
	//P ist type des Speichers
	template<typename T, typename P>
	class baseVectorStorrage : public baseSingleton<P> {
	private:
		//speicher
		std::vector<T*> objects;
	protected:
		//constructor für erstellen von vector
		baseVectorStorrage() {
			objects = std::vector<T*>();
		};
	public:
		//add Object
		static int addOnId(T* obj) {
			objects.push_back(obj);
			return objects.size() - 1;
		}

		//get object by ID
		static T* getById(int ID) {
			if (ID < 0 || ID >= objects.size())
				return NULL;
			return objects[ID];
		}
	};

}
#endif