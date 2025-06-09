#pragma once

#include "net_common.h"
#include "net_message.h"
#include "net_tsdeque.h"

namespace rpc
{
	namespace net
	{
		template<typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:
			// A connection is "owned" by either a server or a client, and its
			// behaviour is slightly different bewteen the two.
			enum class owner
			{
				server,
				client
			};

		public:
			// Constructor: Specify Owner, connect to context, transfer the socket
			//				Provide reference to incoming message queue
			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsdeque<owned_message<T>>& qIn)
				: m_AsioContext(asioContext), m_Socket(std::move(socket)), m_MessagesIn(qIn)
			{
				m_OwnerType = parent;
			}

			virtual ~connection()
			{
			}

			// This ID is used system wide - its how clients will understand other clients
			// exist across the whole system.
			uint32_t GetID() const
			{
				return m_ID;
			}

		public:
			void ConnectToClient(uint32_t uid = 0)
			{
				if (m_OwnerType == owner::server)
				{
					if (m_Socket.is_open())
					{
						m_ID = uid;
						ReadHeader();
					}
				}
			}

			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				// Only clients can connect to servers
				if (m_OwnerType == owner::client)
				{
					// Request asio attempts to connect to an endpoint
					asio::async_connect(m_Socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
						{
							if (!ec)
							{
								ReadHeader();
							}
						});
				}
			}


			void Disconnect()
			{
				if (IsConnected())
					asio::post(m_AsioContext, [this]() { m_Socket.close(); });
			}

			bool IsConnected() const
			{
				return m_Socket.is_open();
			}

			// Prime the connection to wait for incoming messages
			void StartListening()
			{

			}

		public:
			// ASYNC - Send a message, connections are one-to-one so no need to specifiy
			// the target, for a client, the target is the server and vice versa
			void Send(const message<T>& msg)
			{
				asio::post(m_AsioContext,
					[this, msg]()
					{
						// If the queue has a message in it, then we must 
						// assume that it is in the process of asynchronously being written.
						// Either way add the message to the queue to be output. If no messages
						// were available to be written, then start the process of writing the
						// message at the front of the queue.
						bool bWritingMessage = !m_MessagesOut.empty();
						m_MessagesOut.push_back(msg);
						if (!bWritingMessage)
						{
							WriteHeader();
						}
					});
			}



		private:
			// ASYNC - Prime context to write a message header
			void WriteHeader()
			{
				// If this function is called, we know the outgoing message queue must have 
				// at least one message to send. So allocate a transmission buffer to hold
				// the message, and issue the work - asio, send these bytes
				asio::async_write(m_Socket, asio::buffer(&m_MessagesOut.front().header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						// asio has now sent the bytes - if there was a problem
						// an error would be available...
						if (!ec)
						{
							// ... no error, so check if the message header just sent also
							// has a message body...
							if (m_MessagesOut.front().body.size() > 0)
							{
								// ...it does, so issue the task to write the body bytes
								WriteBody();
							}
							else
							{
								// ...it didnt, so we are done with this message. Remove it from 
								// the outgoing message queue
								m_MessagesOut.pop_front();

								// If the queue is not empty, there are more messages to send, so
								// make this happen by issuing the task to send the next header.
								if (!m_MessagesOut.empty())
								{
									WriteHeader();
								}
							}
						}
						else
						{
							// ...asio failed to write the message, we could analyse why but 
							// for now simply assume the connection has died by closing the
							// socket. When a future attempt to write to this client fails due
							// to the closed socket, it will be tidied up.
							std::cout << "[" << m_ID << "] Write Header Fail.\n";
							m_Socket.close();
						}
					});
			}

			// ASYNC - Prime context to write a message body
			void WriteBody()
			{
				// If this function is called, a header has just been sent, and that header
				// indicated a body existed for this message. Fill a transmission buffer
				// with the body data, and send it!
				asio::async_write(m_Socket, asio::buffer(m_MessagesOut.front().body.data(), m_MessagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							// Sending was successful, so we are done with the message
							// and remove it from the queue
							m_MessagesOut.pop_front();

							// If the queue still has messages in it, then issue the task to 
							// send the next messages' header.
							if (!m_MessagesOut.empty())
							{
								WriteHeader();
							}
						}
						else
						{
							// Sending failed, see WriteHeader() equivalent for description :P
							std::cout << "[" << m_ID << "] Write Body Fail.\n";
							m_Socket.close();
						}
					});
			}

			// ASYNC - Prime context ready to read a message header
			void ReadHeader()
			{
				// If this function is called, we are expecting asio to wait until it receives
				// enough bytes to form a header of a message. We know the headers are a fixed
				// size, so allocate a transmission buffer large enough to store it. In fact, 
				// we will construct the message in a "temporary" message object as it's 
				// convenient to work with.
				asio::async_read(m_Socket, asio::buffer(&m_MsgTemporaryIn.header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							// A complete message header has been read, check if this message
							// has a body to follow...
							if (m_MsgTemporaryIn.header.size > 0)
							{
								// ...it does, so allocate enough space in the messages' body
								// vector, and issue asio with the task to read the body.
								m_MsgTemporaryIn.body.resize(m_MsgTemporaryIn.header.size);
								ReadBody();
							}
							else
							{
								// it doesn't, so add this bodyless message to the connections
								// incoming message queue
								AddToIncomingMessageQueue();
							}
						}
						else
						{
							// Reading form the client went wrong, most likely a disconnect
							// has occurred. Close the socket and let the system tidy it up later.
							std::cout << "[" << m_ID << "] Read Header Fail.\n";
							m_Socket.close();
						}
					});
			}

			// ASYNC - Prime context ready to read a message body
			void ReadBody()
			{
				// If this function is called, a header has already been read, and that header
				// request we read a body, The space for that body has already been allocated
				// in the temporary message object, so just wait for the bytes to arrive...
				asio::async_read(m_Socket, asio::buffer(m_MsgTemporaryIn.body.data(), m_MsgTemporaryIn.body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							// ...and they have! The message is now complete, so add
							// the whole message to incoming queue
							AddToIncomingMessageQueue();
						}
						else
						{
							// As above!
							std::cout << "[" << m_ID << "] Read Body Fail.\n";
							m_Socket.close();
						}
					});
			}

			// Once a full message is received, add it to the incoming queue
			void AddToIncomingMessageQueue()
			{
				// Shove it in queue, converting it to an "owned message", by initialising
				// with the a shared pointer from this connection object
				if (m_OwnerType == owner::server)
					m_MessagesIn.push_back({ this->shared_from_this(), m_MsgTemporaryIn });
				else
					m_MessagesIn.push_back({ nullptr, m_MsgTemporaryIn });

				// We must now prime the asio context to receive the next message. It 
				// wil just sit and wait for bytes to arrive, and the message construction
				// process repeats itself. Clever huh?
				ReadHeader();
			}

		protected:
			// Each connection has a unique socket to a remote 
			asio::ip::tcp::socket m_Socket;

			// This context is shared with the whole asio instance
			asio::io_context& m_AsioContext;

			// This queue holds all messages to be sent to the remote side
			// of this connection
			tsdeque<message<T>> m_MessagesOut;

			// This references the incoming queue of the parent object
			tsdeque<owned_message<T>>& m_MessagesIn;

			// Incoming messages are constructed asynchronously, so we will
			// store the part assembled message here, until it is ready
			message<T> m_MsgTemporaryIn;

			// The "owner" decides how some of the connection behaves
			owner m_OwnerType = owner::server;

			uint32_t m_ID = 0;
		};
	}
}