#include "pch.h"
#include "radiant.h"
#include "remote_client.h"

using namespace radiant;
using namespace radiant::simulation;

#define RC_LOG(n)    LOG(simulation.remote_client, n)

static const int MAX_SEQUENCE_SKEW = 10;

RemoteClient::RemoteClient() : 
   _lastSequenceNumberAckd(0),
   _lastSequenceNumberSent(0),
   _lastReadyToRun(true)
{
}

bool RemoteClient::IsClientReadyForUpdates() const
{
   bool ready = _lastSequenceNumberSent - _lastSequenceNumberAckd < MAX_SEQUENCE_SKEW;
   if (ready != _lastReadyToRun) {
      RC_LOG(1) << (ready ? "stopped" : "started") << " buffering client updates. (seq:" << _lastSequenceNumberSent << " ack:" << _lastSequenceNumberAckd << ")";
      _lastReadyToRun = ready;
   }
   return ready;
}

void RemoteClient::SetSequenceNumber(int sequenceNumber)
{
   _lastSequenceNumberSent = sequenceNumber;
}

void RemoteClient::AckSequenceNumber(int sequenceNumber)
{
   _lastSequenceNumberAckd = sequenceNumber;
}

void RemoteClient::FlushSendQueue()
{
   protocol::SendQueue::Flush(send_queue);
}
