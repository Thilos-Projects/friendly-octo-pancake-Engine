#ifndef _CLIENTINTERFACE_
#define _CLIENTINTERFACE_

#include "common.h"
#include "connection.h"

namespace netzwerk
{
	class client_interface
	{
	public:
		client_interface() {}

		virtual ~client_interface(){Disconnect();}

	public:
		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

				m_connection = std::make_shared<connection>(m_context, asio::ip::tcp::socket(m_context), m_qMessagesIn);
					
				m_connection->ConnectToListener(endpoints);

				thrContext = std::thread([this]() { m_context.run(); });
			}
			catch (std::exception& e)
			{
				LOG_F(WARNING, e.what());
				return false;
			}
			return true;
		}

		void Disconnect()
		{
			if(IsConnected())
			{
				m_connection->Disconnect();
			}

			m_context.stop();

			if (thrContext.joinable())
				thrContext.join();

			m_connection.reset();
		}

		bool IsConnected()
		{
			if (m_connection)
				return m_connection->IsConnected();
			else
				return false;
		}

	public:

		void Send(const message<uint32_t>& msg)
		{
			if (IsConnected())
					m_connection->Send(msg);
		}

		tsqueue<owned_message>& Incoming()
		{ 
			return m_qMessagesIn;
		}

	protected:

		asio::io_context m_context;

		std::thread thrContext;

		std::shared_ptr<connection> m_connection;
			
	private:

		tsqueue<owned_message> m_qMessagesIn;
	};
}

#endif