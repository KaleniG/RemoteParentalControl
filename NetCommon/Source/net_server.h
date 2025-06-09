#pragma once

#include "net_common.h"
#include "net_tsdeque.h"
#include "net_message.h"
#include "net_connection.h"

namespace rpc
{
	namespace net
	{
		template<typename T>
		class ServerInterface
		{
		public:
			// Create a server, ready to listen on specified port
			ServerInterface(uint16_t port)
				: m_AsioAcceptor(m_AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{

			}

			virtual ~ServerInterface()
			{
				// May as well try and tidy up
				Stop();
			}

			// Starts the server!
			bool Start()
			{
				try
				{
					// Issue a task to the asio context - This is important
					// as it will prime the context with "work", and stop it
					// from exiting immediately. Since this is a server, we 
					// want it primed ready to handle clients trying to
					// connect.
					WaitForClientConnection();

					// Launch the asio context in its own thread
					m_ThreadContext = std::thread([this]() { m_AsioContext.run(); });
				}
				catch (std::exception& e)
				{
					// Something prohibited the server from listening
					std::cerr << "[SERVER] Exception: " << e.what() << "\n";
					return false;
				}

				std::cout << "[SERVER] Started!\n";
				return true;
			}

			// Stops the server!
			void Stop()
			{
				// Request the context to close
				m_AsioContext.stop();

				// Tidy up the context thread
				if (m_ThreadContext.joinable()) m_ThreadContext.join();

				// Inform someone, anybody, if they care...
				std::cout << "[SERVER] Stopped!\n";
			}

			// ASYNC - Instruct asio to wait for connection
			void WaitForClientConnection()
			{
				// Prime context with an instruction to wait until a socket connects. This
				// is the purpose of an "acceptor" object. It will provide a unique socket
				// for each incoming connection attempt
				m_AsioAcceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket)
					{
						// Triggered by incoming connection request
						if (!ec)
						{
							// Display some useful(?) information
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

							// Create a new connection to handle this client 
							std::shared_ptr<connection<T>> newconn =
								std::make_shared<connection<T>>(connection<T>::owner::server,
									m_AsioContext, std::move(socket), m_MessagesIn);



							// Give the user server a chance to deny connection
							if (OnClientConnect(newconn))
							{
								// Connection allowed, so add to container of new connections
								m_Connections.push_back(std::move(newconn));

								// And very important! Issue a task to the connection's
								// asio context to sit and wait for bytes to arrive!
								m_Connections.back()->ConnectToClient(m_IDCounter++);

								std::cout << "[" << m_Connections.back()->GetID() << "] Connection Approved\n";
							}
							else
							{
								std::cout << "[-----] Connection Denied\n";

								// Connection will go out of scope with no pending tasks, so will
								// get destroyed automagically due to the wonder of smart pointers
							}
						}
						else
						{
							// Error has occurred during acceptance
							std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
						}

						// Prime the asio context with more work - again simply wait for
						// another connection...
						WaitForClientConnection();
					});
			}

			// Send a message to a specific client
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
				// Check client is legitimate...
				if (client && client->IsConnected())
				{
					// ...and post the message via the connection
					client->Send(msg);
				}
				else
				{
					// If we cant communicate with client then we may as 
					// well remove the client - let the server know, it may
					// be tracking it somehow
					OnClientDisconnect(client);

					// Off you go now, bye bye!
					client.reset();

					// Then physically remove it from the container
					m_Connections.erase(
						std::remove(m_Connections.begin(), m_Connections.end(), client), m_Connections.end());
				}
			}

			// Send message to all clients
			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;

				// Iterate through all clients in container
				for (auto& client : m_Connections)
				{
					// Check client is connected...
					if (client && client->IsConnected())
					{
						// ..it is!
						if (client != pIgnoreClient)
							client->Send(msg);
					}
					else
					{
						// The client couldnt be contacted, so assume it has
						// disconnected.
						OnClientDisconnect(client);
						client.reset();

						// Set this flag to then remove dead clients from container
						bInvalidClientExists = true;
					}
				}

				// Remove dead clients, all in one go - this way, we dont invalidate the
				// container as we iterated through it.
				if (bInvalidClientExists)
					m_Connections.erase(
						std::remove(m_Connections.begin(), m_Connections.end(), nullptr), m_Connections.end());
			}

			// Force server to respond to incoming messages
			void Update(size_t nMaxMessages = -1, bool bWait = false)
			{
				if (bWait) m_MessagesIn.wait();

				// Process as many messages as you can up to the value
				// specified
				size_t nMessageCount = 0;
				while (nMessageCount < nMaxMessages && !m_MessagesIn.empty())
				{
					// Grab the front message
					auto msg = m_MessagesIn.pop_front();

					// Pass to message handler
					OnMessage(msg.remote, msg.msg);

					nMessageCount++;
				}
			}

		protected:
			// This server class should override thse functions to implement
			// customised functionality

			// Called when a client connects, you can veto the connection by returning false
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			// Called when a client appears to have disconnected
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{

			}

			// Called when a message arrives
			virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
			{

			}


		protected:
			// Thread Safe Queue for incoming message packets
			tsdeque<owned_message<T>> m_MessagesIn;

			// Container of active validated connections
			std::deque<std::shared_ptr<connection<T>>> m_Connections;

			// Order of declaration is important - it is also the order of initialisation
			asio::io_context m_AsioContext;
			std::thread m_ThreadContext;

			// These things need an asio context
			asio::ip::tcp::acceptor m_AsioAcceptor; // Handles new incoming connection attempts...

			// Clients will be identified in the "wider system" via an ID
			uint32_t m_IDCounter = 10000;
		};
	}
}