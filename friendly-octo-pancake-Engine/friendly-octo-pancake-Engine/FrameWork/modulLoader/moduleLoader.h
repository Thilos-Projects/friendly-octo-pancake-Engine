#ifndef _MODULELOADER_
#define _MODULELOADER_

#include "../Logger_Import/loguru.hpp"
#include <typeinfo>
#include <iostream>
#include "../EntryPoint.h"

namespace FrameWork_moduleLoader {

	class moduleLoader : public FrameWork::mainBehaviour {
	private:
		static FrameWork::behaviourFactoryRegister<moduleLoader> reg;
	public:
		/// <summary>
		/// beim erstellen des behaviours
		/// sollte auf keine anderen behaviours zugreifen und hat keine spefizierte reienfolge
		/// </summary>
		void Awake() {
		}

		/// <summary>
		/// vor dem ersten Update das die classe kennt ausgeführt
		/// </summary>
		void Start() {
		}

		/// <summary>
		/// wird jeden update ausgeführt
		/// </summary>
		void Update() {
		}

		/// <summary>
		/// nach dem letzten update oder vor dem zerstören durch thanos
		/// </summary>
		void onDestroy() {
			LOG_F(WARNING, "destroy");
		}
	};
	FrameWork::behaviourFactoryRegister<moduleLoader> moduleLoader::reg(typeid(moduleLoader).name(), true);

}


#endif