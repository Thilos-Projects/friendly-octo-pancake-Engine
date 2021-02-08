#ifndef _CONNECTION_
#define _CONNECTION_

#include "common.h"
#include "tsqueue.h"
#include "message.h"

namespace netzwerk
{
	class connection : public std::enable_shared_from_this<connection>
	{
	public:

		uint32_t myId = 0;
		uint32_t otherId = 0;

		connection(asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<message<uint32_t>>& qIn)
			: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
		{
		}

		virtual ~connection() {}

		void ConnectFromListener()
		{
			if (m_socket.is_open())
			{
				ReadHeader();
			}
		}

		void RefuseFromListener() {
			if (IsConnected())
				asio::post(m_asioContext, [this]() { m_socket.close(); });
		}

		void ConnectToListener(const asio::ip::tcp::resolver::results_type& endpoints)
		{
			asio::async_connect(m_socket, endpoints,
				[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
				{
					if (!ec)
					{
						ReadHeader();
					}
				});
		}

		void Disconnect()
		{
			if (IsConnected())
				asio::post(m_asioContext, [this]() { m_socket.close(); });
		}

		bool IsConnected() const
		{
			return m_socket.is_open();
		}

		void Send(const message<uint32_t>& msg)
		{
			if(IsConnected())
				asio::post(m_asioContext,
					[this, msg]()
					{
						if (m_qMessagesOut.empty())
						{
							m_qMessagesOut.push_back(msg);
							WriteHeader();
						}
						else
							m_qMessagesOut.push_back(msg);
					});
		}

	private:
		void WriteHeader()
		{
			asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<uint32_t>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_qMessagesOut.front().body.size() > 0)
							WriteBody();
						else
						{
							m_qMessagesOut.pop_front();

							if (!m_qMessagesOut.empty())
								WriteHeader();
						}
					}
					else
					{
						m_socket.close();	//im zweifel ändern
					}
				});
		}

		void WriteBody()
		{
			asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_qMessagesOut.pop_front();

						if (!m_qMessagesOut.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						m_socket.close();	//im zweifel ändern
					}
				});
		}

		void ReadHeader()
		{
			asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<uint32_t>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_msgTemporaryIn.header.size > 0)
						{
							m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
							ReadBody();
						}
						else
							AddToIncomingMessageQueue();
					}
					else
					{
						m_socket.close();	//im zweifel ändern
					}
				});
		}

		void ReadBody()
		{
			asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						m_socket.close();	//im zweifel ändern
					}
				});
		}

		void AddToIncomingMessageQueue()
		{				
			m_qMessagesIn.push_back(m_msgTemporaryIn);

			ReadHeader();
		}

	protected:

		asio::ip::tcp::socket m_socket;

		asio::io_context& m_asioContext;

		tsqueue<message<uint32_t>> m_qMessagesOut;

		tsqueue<message<uint32_t>>& m_qMessagesIn;

		message<uint32_t> m_msgTemporaryIn;
	};
}

#endif