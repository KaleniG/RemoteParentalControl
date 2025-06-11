#pragma once

#include <rpc_core.h>
#include <rpc_net.h>

#include "Core/Common.h"

namespace rpc
{
  class ParentClient : public net::ServerInterface<net::message_type>
  {
  public:
    ParentClient(uint16_t port);

    void ChangeFrameQuality(uint32_t quality);

    bool ClientConnected() const { return m_ConnectedClient != nullptr; }
    bool NewFrameAvailable();

  protected:
    bool OnClientConnect(std::shared_ptr<net::connection<net::message_type>> client) override;
    void OnClientDisconnect(std::shared_ptr<net::connection<net::message_type>> client) override;
    void OnMessage(std::shared_ptr<net::connection<net::message_type>> client, net::message<net::message_type>& msg) override;

  private:
    std::shared_ptr<net::connection<net::message_type>> m_ConnectedClient = nullptr;
    std::atomic<bool> m_NewFrameAvailable = false;
  };
}