#ifndef _SERVER_
#define _SERVER_

#include "connection.h"
#include "tsqueue.h"
#include "message.h"

namespace netzwerk
{
	class netzwerkBasik
	{

	public:
		enum class connectionResponseBites : uint32_t {
			Close = 1,
			Denay = 2,
			WelcomeMessage = 4
		};

		typedef bool(*onConnectFunkt)(uint32_t otherID, std::string otherIP, uint16_t port);
		typedef void(*onDisconectFunkt)(uint32_t otherID);
		typedef void(*onMessageFunkt)(message<uint32_t> message);

		netzwerkBasik(uint16_t port, onConnectFunkt onConnect, onDisconectFunkt onDisconnect, onMessageFunkt onMessage)
			: m_asioAcceptor(m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{
			this->onConnect = onConnect;
			this->onDisconnect = onDisconnect;
			this->onMessage = onMessage;
		}

		virtual ~netzwerkBasik()
		{
			Stop();
		}
		
		bool Start()
		{
			openConnection();
		}
		
		void Stop()
		{
			m_context.stop();

			if (m_threadContext.joinable()) m_threadContext.join();

			LOG_F(INFO, "Server Stopped");
		}

		void WaitForOtherConnection()
		{
			m_asioAcceptor.async_accept(
				[this](std::error_code ec, asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						uint32_t ID = newOtherID();

						std::string ip = socket.remote_endpoint().address().to_string();
						uint32_t port = socket.remote_endpoint().port();

						std::shared_ptr<connection> newconn = std::make_shared<connection>(m_context, std::move(socket), m_qMessagesIn);

						if (onConnect(ID,ip,port))
						{
							auto temp = m_connections.insert(std::make_pair(ID, std::move(newconn))).first->second;
							temp->myId = myID;
							temp->otherId = ID;
							temp->ConnectFromListener();

							LOG_F(INFO, "New Connection with ID %d from IP %s approved",ID, ip.c_str());
						}
						else
						{
							newconn->RefuseFromListener();
							LOG_F(INFO, "New Connection with ID %d from IP %s denied", ID, ip.c_str());
						}
					}
					else
					{
						LOG_F(INFO, "New Connection Failed");
					}

					WaitForOtherConnection();
				});
		}
		
		void MessageClient(const message<uint32_t>& msg, bool setSenderID = true)
		{
			message<uint32_t> copy = message<uint32_t>(msg);	//auf daten korrektheit prüfen
			if (setSenderID)
				copy.header.sourceID = myID;
			if (msg.header.destinationID == 0)
				for (auto i = m_connections.begin(); i != m_connections.end(); i++) {
					copy.header.destinationID = i->first;
					MessageClient(i->second, copy);
				}
			else {
				std::shared_ptr<connection> other = m_connections.find(msg.header.destinationID)->second;
				if (!other)
					return;
				MessageClient(other, copy);
			}
		}

		void Update(size_t nMaxMessages = -1, bool bWait = false)
		{
			if (bWait) m_qMessagesIn.wait();

			size_t nMessageCount = 0;
			while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
			{
				auto msg = m_qMessagesIn.pop_front();

				if (msg.msg.header.destinationID == myID)
					onMessage(msg.msg);
				else
					MessageClient(msg.msg, false);

				nMessageCount++;
			}
			DeleteUnusedOthers();
		}

	protected:
		bool openConnection() {
			try
			{
				WaitForOtherConnection();

				m_threadContext = std::thread([this]() { m_context.run(); });
			}
			catch (std::exception& e)
			{
				LOG_F(ERROR, "Server Start Failed with : %s", e.what());
				return false;
			}

			LOG_F(INFO, "Server Startet");
			return true;
		}

		void MessageClient(std::shared_ptr<connection> client, const message<uint32_t>& msg)
		{
			if (client && client->IsConnected())
				client->Send(msg);
			else
			{
				onDisconnect(client->otherId);
				toDelete.push_back(client->otherId);
				client.reset();
			}
		}

		void DeleteUnusedOthers() {
			if (!toDelete.empty()) {
				for (int i = 0; i < toDelete.size(); i++)
					m_connections.erase(toDelete[i]);
				toDelete.clear();
			}
		}

		uint32_t newOtherID() {
			uint32_t ID;
			do {
				ID = rand();
			} while (m_connections.find(ID) != m_connections.end() || ID == 0 || ID == myID);
			return ID;
		}

		tsqueue<owned_message> m_qMessagesIn;

		std::map<uint32_t, uint32_t> m_childChilds;
		std::map<uint32_t, std::shared_ptr<connection>> m_connections;
		std::vector<uint32_t> toDelete;

		std::shared_ptr<connection> myConnection;

		asio::io_context m_context;
		std::thread m_threadContext;

		asio::ip::tcp::acceptor m_asioAcceptor;

	private:
		onConnectFunkt onConnect;
		onDisconectFunkt onDisconnect;
		onMessageFunkt onMessage;

		uint32_t myID = 0;
	};
}

#endif