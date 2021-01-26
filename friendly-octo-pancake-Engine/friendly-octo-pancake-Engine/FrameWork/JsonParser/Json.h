#ifndef _JSON_H_
#define _JSON_H_

#include <string>
#include <stack>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>

namespace JsonParser {
	/// <summary>
	/// wandelt einen json string in ein json objekt um das anschließend ausgelesen werden kann
	/// </summary>
	class JsonParser {
	public:
		class JsonObject;
		class JsonArray;
		/// <summary>
		/// alle objekt typen
		/// </summary>
		enum objectTypes {
			String,
			Number,
			Boolean,
			Null,
			Object,
			Array
		};

		/// <summary>
		/// überspringt leerzeichen tabs und newlines in einem itterator
		/// </summary>
		/// <param name="it">der itterator in dem übersprungen werden soll</param>
		/// <param name="end">das ende des itterators</param>
		static void moveToNextSymbol(std::string::iterator& it, std::string::iterator& end) {
			for (; it != end && (*it == ' ' || *it == '\t' || *it == '\n'); ++it) {
			}
		}

		/// <summary>
		/// ließt einen string bis zu einem " ein und übersprinngt " am anfang und am ende
		/// </summary>
		/// <param name="it">der itterator in dem gelesen werden soll</param>
		/// <param name="end">das ende des itterators</param>
		/// <returns>der ergebnis string ohne " als zeichen</returns>
		static std::string readString(std::string::iterator& it, std::string::iterator& end) {
			std::string toRead = "";
			if (*it == '"')
				++it;
			for (; it != end && (*it != '"'); ++it)
				toRead += *it;
			++it;
			return toRead;
		}

		/// <summary>
		/// ließt den objekt typ des kommenden objekts ohne ein zeichen zu überspringen
		/// </summary>
		/// <param name="it">der itterator in dem übersprungen werden soll</param>
		/// <param name="end">das ende des itterators</param>
		/// <returns>der type des nächsten objektes</returns>
		static objectTypes getObjectType(std::string::iterator& it, std::string::iterator& end) {
			switch (*it) {
			case '"':
				++it;
				return objectTypes::String;
			case '{':
				++it;
				return objectTypes::Object;
			case '[':
				++it;
				return objectTypes::Array;
			case 't': case 'f':
				return objectTypes::Boolean;
			case 'n':
				return objectTypes::Null;
			default:
				if (isdigit(*it))
					return objectTypes::Number;
			}
		}

		struct JsonObject {
		private:
			objectTypes type;
			std::string simpeData;

			std::vector<JsonObject> arrayData;
			std::map<std::string, JsonObject> atributes;

		public:
			JsonObject() {};

			JsonObject(objectTypes _type) {
				type = _type;
				if (_type == objectTypes::Array)
					arrayData = std::vector<JsonObject>();
				else if (_type == objectTypes::Object)
					atributes = std::map<std::string, JsonObject>();
				else
					simpeData = "";
			}

			void parse(std::string::iterator& it, std::string::iterator& end) {
				moveToNextSymbol(it, end);

				switch (type) {
				case objectTypes::Object:
				{
					while (it != end) {
						std::string key = readString(it, end);
						moveToNextSymbol(it, end);
						if (*it != ':') {

						}
						++it;
						moveToNextSymbol(it, end);
						atributes.insert(std::pair<std::string, JsonObject>(key, JsonObject(getObjectType(it, end))));
						atributes[key].parse(it, end);
						moveToNextSymbol(it, end);
						if (*it == ',')
							++it;
						else {
							++it;
							break;
						}
						moveToNextSymbol(it, end);
					}
				}break;
				case objectTypes::String:
				{
					simpeData = readString(it, end);
				}break;
				case objectTypes::Boolean:
				{
					if (*it == 't') {
						simpeData = "true";
					}
					else if (*it == 'f') {
						simpeData = "false";
						++it;
					}
					else {
						std::cout << "hä " << *it << std::endl;
					}

					++it;
					++it;
					++it;
					++it;
				}break;
				case objectTypes::Number:
				{
					for (; it != end && isdigit(*it); ++it)
						simpeData += *it;
				}break;
				case objectTypes::Null:
				{
					simpeData = "null";
					++it;
					++it;
					++it;
					++it;
				}break;
				case objectTypes::Array:
				{
					while (it != end) {
						moveToNextSymbol(it, end);
						arrayData.push_back(JsonObject(getObjectType(it, end)));
						arrayData[arrayData.size() - 1].parse(it, end);
						moveToNextSymbol(it, end);
						if (*it == ',')
							++it;
						else {
							++it;
							break;
						}
						moveToNextSymbol(it, end);
					}
				}break;
				}
			};

			JsonObject& operator[](std::string idx) {
				if (type == objectTypes::Object)
					return atributes[idx];
				if (type == objectTypes::Array)
					return arrayData[std::stoi(idx)];
				std::cerr << "ist nicht indizierbar" << std::endl;
				throw "no value";
			}

			std::vector<std::string> possibleValues() {
				std::vector<std::string> toRet = std::vector<std::string>();
				if (type == objectTypes::Object) {
					for (std::map<std::string, JsonObject>::iterator it = atributes.begin(); it != atributes.end(); ++it)
						toRet.push_back((*it).first);
				}
				if (type == objectTypes::Array) {
					for (int i = 0; i < arrayData.size(); i++)
						toRet.push_back(std::to_string(i));
				}
				return toRet;
			}

			objectTypes getType() { return type; };
			std::string getData() { return simpeData; };

		};

		static JsonObject parse(std::string jsonString) {
			std::string::iterator end = jsonString.end();
			std::string::iterator it = jsonString.begin();
			return parse(it, end);
		}
		static JsonObject parse(std::string::iterator it, std::string::iterator end) {
			JsonObject startObject = JsonObject(getObjectType(it, end));
			startObject.parse(it, end);
			return startObject;
		}
	};
}

#endif