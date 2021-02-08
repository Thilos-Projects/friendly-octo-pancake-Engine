#ifndef _NETZWERKKERNAL_
#define _NETZWERKKERNAL_

#include "common.h"
#include "connection.h"
#include "message.h"

namespace netzwerk {

	class NetzwerkKernal
	{
	public:

		typedef bool(*onConnectFunkt)(uint32_t otherID, std::string otherIP, uint16_t port);
		typedef void(*onMessageType)(message<uint32_t> msg);
		typedef void(*onIdType)(uint32_t id);

		NetzwerkKernal(onMessageType onMessage, onMessageType onDestinationNotFound, onIdType onConnectionRemoved, onIdType onConnectionAdded, onConnectFunkt onConnect) :
			m_acceptor(m_context){
			this->onMessage = onMessage;
			this->onDestinationNotFound = onDestinationNotFound;
			this->onConnectionRemoved = onConnectionRemoved;
			this->onConnectionAdded = onConnectionAdded;
			this->onConnect = onConnect;
			this->running = false;
			this->myId = 0;
			this->serverId = 0;
		}

		/// <summary>
		/// ist als bit flag angelegt
		/// manche states sind bit übergreifend und entsprechend nicht nur ein bit
		///  z.b. kann es keine client entfernung geben die nicht für den server ist
		/// </summary>
		enum class messageBites : uint32_t {
			serverMessage = 1,
			destroyConnection = 3,
			newConnection = 5
		};

		void Start(uint16_t myPort) {
			running = true;
			m_acceptor = asio::ip::tcp::acceptor(m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), myPort));
			idsFromServer = std::vector<uint32_t>();
			connectionToServer = nullptr;
			myId = buildId();
			try
			{
				WaitForClientConnection();

				m_thrContext = std::thread(
					[this]() {
						m_context.run(); 
					});
				m_thrUpdateOut = std::thread(
					[this]() {
						while (running) {
							sendMessageFromOutMessage();

						}
					});
				m_thrUpdateIn = std::thread(
					[this]() {
						while (running) {
							handleIncommingMessages();
						}
					});
				m_thrUpdateRem = std::thread(
					[this]() {
						while (running) {
							handleRemoveClients();
						}
					});
			}
			catch (std::exception& e)
			{
				LOG_F(ERROR, "Server Start Failed with : %s", e.what());
				return;
			}


			LOG_F(INFO, "Server Startet");
		}

		void Start(std::string otherIp, uint16_t serverPort, uint16_t myPort) {
			running = true;
			m_acceptor = asio::ip::tcp::acceptor(m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), myPort));
			idsFromServer = std::vector<uint32_t>();
			try
			{
				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(otherIp, std::to_string(serverPort));

				connectionToServer = std::make_shared<connection>(m_context, asio::ip::tcp::socket(m_context), m_msgIn);

				connectionToServer->ConnectToListener(endpoints);

				WaitForClientConnection();

				m_thrContext = std::thread(
					[this]() { 
						m_context.run(); 
					});
				m_thrUpdateOut = std::thread(
					[this]() {
						while (running) {
							sendMessageFromOutMessage();

						}
					});
				m_thrUpdateIn = std::thread(
					[this]() {
						while (running) {
							handleIncommingMessages();
						}
					});
				m_thrUpdateRem = std::thread(
					[this]() {
						while (running) {
							handleRemoveClients();
						}
					});
			}
			catch (std::exception& e)
			{
				LOG_F(WARNING, e.what());
				return;
			}
		}

		/// <summary>
		/// stoppt das system
		/// </summary>
		void Stop() {
			running = false;

			m_context.stop();

			if (m_thrContext.joinable()) m_thrContext.join();
			if (m_thrUpdateOut.joinable()) m_thrUpdateOut.join();
			if (m_thrUpdateIn.joinable()) m_thrUpdateIn.join();
			if (m_thrUpdateRem.joinable()) m_thrUpdateRem.join();
		}

		/// <summary>
		/// beschreibt ob ein server läuft
		/// </summary>
		/// <returns></returns>
		bool isRunning() {
			return running;
		}

		/// <summary>
		/// fügt eine message zu den zu sendenen messages hinzu
		/// </summary>
		/// <param name="msg">die message</param>
		void addMessageToMessageOut(message<uint32_t> msg) {
			m_msgOut.push_back(msg);
		}

	private:

		/// <summary>
		/// primt asio mit der auf client warten aufruf welcher auf dieser funktion recursiv ist
		/// </summary>
		void WaitForClientConnection()
		{
			m_acceptor.async_accept(
				[this](std::error_code ec, asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						uint32_t Id = buildId();
						std::string ip = socket.remote_endpoint().address().to_string();
						uint32_t port = socket.remote_endpoint().port();

						std::shared_ptr<connection> newconn = std::make_shared<connection>(m_context, std::move(socket), m_msgIn);

						if (onConnect(Id, ip, port))
						{
							newconn->myId = myId;
							newconn->otherId = Id;
							newconn->ConnectFromListener();

							registerClient(newconn);

							LOG_F(INFO, "New Connection with ID %d from IP %s approved", Id, ip.c_str());
						}
						else
						{
							newconn->RefuseFromListener();
							LOG_F(INFO, "New Connection with ID %d from IP %s denied", Id, ip.c_str());
						}
					}
					else
					{
						LOG_F(INFO, ec.message().c_str());
					}

					WaitForClientConnection();
				});
		}

		/// <summary>
		/// baut eine neue im system noch nicht bekannte id die nicht 0 oder 1 ist
		/// 0 ist global
		/// 1 ist direkt nachricht wirn nie weitergeleitet
		/// </summary>
		/// <returns>die id</returns>
		uint32_t buildId() {
			uint32_t id;
			while (true) {
				id = rand();
				if (id == 0 || id == 1 || id == myId)
					continue;
				for (int i = 0; i < idsFromServer.size(); i++)
					if (id == idsFromServer[i])
						continue;
				auto t = clientToDirectClient.find(id);
				if (t != clientToDirectClient.end())
					continue;
				return id;
			}
		}

		/// <summary>
		/// registriert eine client ID in der serverseite des netzwerks
		/// </summary>
		/// <param name="clientID">die client id</param>
		void registerClient(uint32_t clientID) {
			idsFromServer.push_back(clientID);
		}

		/// <summary>
		/// regestriert einen sub client in einem client
		/// </summary>
		/// <param name="subClientID">die subclient id</param>
		/// <param name="clientID">die client id</param>
		void registerClient(uint32_t subClientID, uint32_t clientID) {
			clientToDirectClient.insert(std::make_pair(subClientID, clientID));
		}

		/// <summary>
		/// registriert einen directen client
		/// </summary>
		/// <param name="conn">die connection zu dem client !muss fertig eingerichtet sein!</param>
		void registerClient(std::shared_ptr<connection>& conn) {

			auto msg = ServerConnectionAddedMessage(0);
			msg << conn->otherId;
			addMessageToMessageOut(msg);

			msg = ServerConnectionAddedMessage(1);
			msg << conn->otherId;
			sendMessageDirect(conn, msg);

			msg = ServerConnectionAddedMessage(conn->otherId);
			for (int i = 0; i < idsFromServer.size(); i++)
				msg << idsFromServer[i];
			for (auto it = clientToDirectClient.begin(); it != clientToDirectClient.end(); it++)
				msg << it->first;

			directClients.insert(std::make_pair(conn->otherId, conn));
			clientToDirectClient.insert(std::make_pair(conn->otherId, conn->otherId));

			addMessageToMessageOut(msg);
		}

		/// <summary>
		/// wartet bis ein element in der out Que ist und verarbeitet dann alle anwesenden
		/// das warten sleeped den thread
		/// </summary>
		void sendMessageFromOutMessage() {
			m_msgOut.wait();

			while (!m_msgOut.empty())
			{
				sendMessage(m_msgOut.pop_front());
			}
		}

		/// <summary>
		/// wartet bis ein element in der in Que ist und verarbeitet dann alle anwesenden
		/// das warten sleeped den thread
		/// </summary>
		void handleIncommingMessages() {
			m_msgIn.wait();

			while (!m_msgIn.empty())
			{
				reciveMessage(m_msgIn.pop_front());
			}
		}

		/// <summary>
		/// verarbeitet eine message falls diese an diesen client muss sonst wird an sendMessage Weitergeleited
		/// </summary>
		/// <param name="msg">die message</param>
		void reciveMessage(message<uint32_t> msg) {
			if (msg.header.destinationID == 1 || msg.header.destinationID == myId) {
				useMessage(msg);
				return;
			}
			if (msg.header.destinationID == 0) {
				sendMessage(msg);
				useMessage(msg);
				return;
			}
			sendMessage(msg);
		}

		/// <summary>
		/// versendet eine nachricht im netzwerk an eine beliebige in der msg beschriebene source
		/// </summary>
		/// <param name="msg">die message</param>
		void sendMessage(message<uint32_t> msg) {
			if (msg.header.destinationID == 0) {
				if(connectionToServer)
					sendMessageDirect(connectionToServer, msg);
				for (auto it = directClients.begin(); it != directClients.end(); it++)
					sendMessageDirect(it->second, msg);

				return;
			}

			for (int i = 0; i < idsFromServer.size(); i++)
				if (idsFromServer[i] == msg.header.destinationID) {
					if(connectionToServer)
						sendMessageDirect(connectionToServer, msg);
					return;
				}

			auto t0 = clientToDirectClient.find(msg.header.destinationID);
			if (t0 != clientToDirectClient.end()) {
				auto t1 = directClients.find(t0->second);
				if (t1 != directClients.end()) {
					sendMessageDirect(t1->second, msg);
					return;
				}
			}

			onDestinationNotFound(msg);
		}

		/// <summary>
		/// versendet nachrichten
		/// prüft allerdings ob die fremd id der verwisende connection die source ist und blockt in diesem fall
		/// sollte die nachricht die destination 0 oder 1 sein wird die source auf die eigene id der connection gelegt
		/// </summary>
		/// <param name="conn"> die verbindung auf der gesendet werden soll</param>
		/// <param name="msg">die nachricht</param>
		void sendMessageDirect(std::shared_ptr<connection> conn, message<uint32_t> msg) {
			if (conn->otherId == msg.header.sourceID)
				return;
			if (msg.header.destinationID == 0 || msg.header.destinationID == 1)
				msg.header.sourceID = conn->myId;
			if (conn && conn->IsConnected())
				conn->Send(msg);
			else
				clientsToRemove.push_back(conn->otherId);
		}

		/// <summary>
		/// wartet auf zu entfernende items und
		/// entfernt alle items in der clientsToRemove que
		/// </summary>
		void handleRemoveClients() {

			clientsToRemove.wait();

			uint32_t id;
			while (!clientsToRemove.empty()) {
				id = clientsToRemove.pop_front();
				if (id == myId)
					removeSelf();
				else
					removeClient(id);
			}
		}

		/// <summary>
		/// entfernt alle verweise auf verbindungen und deren ids
		/// </summary>
		void removeSelf() {
			connectionToServer.reset();
			connectionToServer = nullptr;
			idsFromServer.clear();
			for (int i = 0; i < directClients.size(); i++)
				directClients[i].reset();
			directClients.clear();
			clientToDirectClient.clear();
		}

		/// <summary>
		/// entfernt einen kind client aus dem system und benachrichtigt das system darüber
		/// </summary>
		/// <param name="clientId">die id des clients</param>
		void removeClient(uint32_t clientId) {
			LOG_F(WARNING, "connectionLost on ID: %d", clientId);
			
			//wenn client direkt
			auto er0 = directClients.find(clientId);
			if (er0 != directClients.end()) {

				message<uint32_t> msg = ServerConnectionClosedMessage(0);
				msg << clientId;

				std::vector<uint32_t> toDestroy = std::vector<uint32_t>();
				for (auto it = clientToDirectClient.begin(); it != clientToDirectClient.end(); it++) {
					if (it->second == clientId) {
						toDestroy.push_back(it->first);
						msg << it->first;
					}
				}

				for (int i = 0; i < toDestroy.size(); i++)
					clientToDirectClient.erase(toDestroy[i]);
				
				directClients.erase(clientId);

				sendMessage(msg);

				return;
			}

			//wenn client server
			for(int i = 0; i < idsFromServer.size(); i++)
				if (idsFromServer[i] == clientId) {
					idsFromServer.erase(idsFromServer.begin() + i);
					return;
				}

			//wenn client sub client
			clientToDirectClient.erase(clientId);
		}

		/// <summary>
		/// nimmt eine message entgegen die für diese instance bestimmt ist
		/// </summary>
		/// <param name="msg">die message</param>
		void useMessage(message<uint32_t> msg) {
			if (msg.header.sourceID == myId)
				return;

			if (msg.header.id & (uint32_t)messageBites::serverMessage) {
				if (msg.header.id == (uint32_t)messageBites::destroyConnection) {
					uint32_t tempId;
					do {
						msg >> tempId;
						clientsToRemove.push_back(tempId);
						onConnectionRemoved(tempId);
					} while (msg.body.size() >= sizeof(uint32_t));
					return;
				}
				if (msg.header.id == (uint32_t)messageBites::newConnection) {
						if (msg.header.destinationID == 1) {
							msg >> myId;
							idsFromServer.push_back(msg.header.sourceID);
							connectionToServer->myId = myId;
							connectionToServer->otherId = msg.header.sourceID;
							serverId = msg.header.sourceID;
							onConnectionAdded(myId);
						}
						else {
							if (msg.header.sourceID == serverId) {
								uint32_t tempId;
								do {
									msg >> tempId;
									onConnectionAdded(tempId);
									registerClient(tempId);
								} while (msg.body.size() >= sizeof(uint32_t));
							}
							else {
								uint32_t tempId;
								do {
									msg >> tempId;
									registerClient(tempId, msg.header.sourceID);
									onConnectionAdded(tempId);
								} while (msg.body.size() >= sizeof(uint32_t));
							}
						}
					return;
				}
			}
			else
				onMessage(msg);
		}

		/// <summary>
		/// benachrichtigt das ein client entfernt wurde
		/// die id des clients ist im body
		/// </summary>
		/// <param name="destination"></param>
		/// <returns></returns>
		message<uint32_t> ServerConnectionClosedMessage(uint32_t destination) {
			message<uint32_t> msg = message<uint32_t>();
			msg.header.destinationID = destination;
			msg.header.sourceID = myId;
			msg.header.id = (uint32_t)messageBites::destroyConnection;
			return msg;
		}
		
		/// <summary>
		/// benachrichtigt das ein client hinzugefügt wurde
		/// die id des clients ist im body
		/// </summary>
		/// <param name="destination"></param>
		/// <returns></returns>
		message<uint32_t> ServerConnectionAddedMessage(uint32_t destination) {
			message<uint32_t> msg = message<uint32_t>();
			msg.header.destinationID = destination;
			msg.header.sourceID = myId;
			msg.header.id = (uint32_t)messageBites::newConnection;
			return msg;
		}

		uint32_t myId;

		bool running;

		std::vector<uint32_t> idsFromServer;
		uint32_t serverId;
		std::shared_ptr<connection> connectionToServer;

		std::map<uint32_t, uint32_t> clientToDirectClient;
		std::map<uint32_t, std::shared_ptr<connection>> directClients;

		onMessageType onMessage;
		onMessageType onDestinationNotFound;
		onIdType onConnectionRemoved;
		onIdType onConnectionAdded;
		onConnectFunkt onConnect;

		asio::io_context m_context;
		std::thread m_thrContext;
		std::thread m_thrUpdateOut;
		std::thread m_thrUpdateIn;
		std::thread m_thrUpdateRem;

		asio::ip::tcp::acceptor m_acceptor;

		tsqueue<message<uint32_t>> m_msgIn;
		tsqueue<message<uint32_t>> m_msgOut;

		tsqueue<uint32_t> clientsToRemove;
	};
}
#endif