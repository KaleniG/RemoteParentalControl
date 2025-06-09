#pragma once

#include "net_connection.h"
#include "net_message.h"
#include "net_tsdeque.h"
#include "net_common.h"

namespace rpc
{
	namespace net
	{
		template <typename T>
		class ClientInterface
		{
		public:
			ClientInterface()
			{

			}

			virtual ~ClientInterface()
			{
				Disconnect();
			}

		public:
			// Connect to server with hostname/ip-address and port
			bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					// Resolve hostname/ip-address into tangiable physical address
					asio::ip::tcp::resolver resolver(m_Context);
					asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

					// Create connection
					m_Connection = std::make_unique<connection<T>>(connection<T>::owner::client, m_Context, asio::ip::tcp::socket(m_Context), m_MessagesIn);

					// Tell the connection object to connect to server
					m_Connection->ConnectToServer(endpoints);

					// Start Context Thread
					m_ContextThread = std::thread([this]() { m_Context.run(); });
				}
				catch (std::exception& e)
				{
					std::cerr << "Client Exception: " << e.what() << "\n";
					return false;
				}
				return true;
			}

			// Disconnect from server
			void Disconnect()
			{
				// If connection exists, and it's connected then...
				if (IsConnected())
				{
					// ...disconnect from server gracefully
					m_Connection->Disconnect();
				}

				// Either way, we're also done with the asio context...				
				m_Context.stop();
				// ...and its thread
				if (m_ContextThread.joinable())
					m_ContextThread.join();

				// Destroy the connection object
				m_Connection.release();
			}

			// Check if client is actually connected to a server
			bool IsConnected()
			{
				if (m_Connection)
					return m_Connection->IsConnected();
				else
					return false;
			}

		public:
			// Send message to server
			void Send(const message<T>& msg)
			{
				if (IsConnected())
					m_Connection->Send(msg);
			}

			// Retrieve queue of messages from server
			tsdeque<owned_message<T>>& Incoming()
			{
				return m_MessagesIn;
			}

		protected:
			// asio context handles the data transfer...
			asio::io_context m_Context;
			// ...but needs a thread of its own to execute its work commands
			std::thread m_ContextThread;
			// The client has a single instance of a "connection" object, which handles data transfer
			std::unique_ptr<connection<T>> m_Connection;

		private:
			// This is the thread safe queue of incoming messages from server
			tsdeque<owned_message<T>> m_MessagesIn;
		};
	}
}