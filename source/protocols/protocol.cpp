#include "radiant.h"
#include <iostream>
#include <iomanip> 
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include "protocol.h"

using namespace ::radiant;
using namespace ::radiant::protocol;

SendQueuePtr SendQueue::Create(boost::asio::ip::tcp::socket& s, const char* endpoint)
{
   return std::make_shared<SendQueue>(s, endpoint);
}

void SendQueue::Flush(std::shared_ptr<SendQueue> sendQueue)
{
   ASSERT(sendQueue);
   
   if (!sendQueue) {
      return;
   }

   if (sendQueue->_lastBufferCompleted >= sendQueue->_lastBufferWritten - sendQueue->_maxBuffersInFlight) {
      auto &queue = sendQueue->_queue;

      while (!queue.empty()) {
         auto buffer = queue.front();
         queue.pop_front();
         auto writeFinished = [sendQueue, buffer](const boost::system::error_code& error, size_t bytes_transferred) {
            ASSERT(bytes_transferred == buffer->size);
            ASSERT(!error);
            sendQueue->_bufferedBytes -= buffer->size;
            NETWORK_LOG(3) << sendQueue->_endpoint << " finished sending buffer of size " << buffer->size << "(bytes pending: " << sendQueue->_bufferedBytes << ")";
            sendQueue->_freelist.push(buffer);
            sendQueue->_lastBufferCompleted++;
         };
         sendQueue->_bufferedBytes += buffer->size;
         NETWORK_LOG(3) << sendQueue->_endpoint << " sending buffer of size " << buffer->size << "(bytes pending: " << sendQueue->_bufferedBytes << ")";
         sendQueue->socket_.async_send(boost::asio::buffer(buffer->data, buffer->size), writeFinished);
         sendQueue->_lastBufferWritten++;
      }
   } else {
      NETWORK_LOG(2) << sendQueue->_endpoint << " sendqueue too far ahead of receiver; skipping flush.";
   }
}

void SendQueue::Push(google::protobuf::MessageLite const& msg)
{
   unsigned int msgSize = msg.ByteSize();
   unsigned int requiredSize = msgSize + sizeof(uint32);
   BufferPtr buffer;

   ASSERT(requiredSize < SEND_BUFFER_SIZE);

   if (!_queue.empty() && (SEND_BUFFER_SIZE - _queue.back()->size > requiredSize)) {
      buffer = _queue.back();
   } else {
      if (!_freelist.empty()) {
         NETWORK_LOG(7) << _endpoint << " popping recycled send buffer off free list";
         buffer = _freelist.top();
         _freelist.pop();
      } else {
         NETWORK_LOG(2) << _endpoint << " allocating new send buffer (total: " << _queue.size() << ")";
         buffer = std::make_shared<Buffer>();
      }
      buffer->size = 0;
      _queue.push_back(buffer);
   }
   ASSERT(!_queue.empty() && buffer == _queue.back());
   int bufferSize = buffer->size;
   unsigned int remaining = sizeof(buffer->data) - bufferSize;
   char *base = buffer->data + bufferSize;

   ASSERT(remaining > requiredSize);
   {
      google::protobuf::io::ArrayOutputStream output(base, remaining);
      {
         google::protobuf::io::CodedOutputStream encoder(&output);

         encoder.WriteLittleEndian32(msgSize);
         msg.SerializeToCodedStream(&encoder);

         unsigned int written = static_cast<int>(encoder.ByteCount());
         ASSERT(written <= requiredSize);
         ASSERT(bufferSize + written <= SEND_BUFFER_SIZE);
         buffer->size += written;
      }
   }
}


RecvQueue::RecvQueue(boost::asio::ip::tcp::socket& s, const char* endpoint) : 
   socket_(s),
   readPending_(false),
   readBuf_(ReadBufferSize),
   endpoint_(endpoint)
{
}

void RecvQueue::Read()
{
   ASSERT(!readPending_);

   readBuf_.compress();
   if (readBuf_.remaining() > ReadLowWaterMark) {
      readPending_ = true;

      auto read_handler = std::bind(&RecvQueue::HandleRead, this, shared_from_this(), std::placeholders::_1, std::placeholders::_2);
      socket_.async_receive(boost::asio::buffer((void *)readBuf_.nextdata(), readBuf_.remaining()), read_handler);
   } else {
      NETWORK_LOG(3) << endpoint_ << " buffer only has " << readBuf_.remaining() << " in RecvQueue::Read.  Waiting for next process loop...";
   }
}

void RecvQueue::HandleRead(RecvQueuePtr q, const boost::system::error_code& e, std::size_t bytes_transferred)
{
   ASSERT(readPending_); 
   readPending_ = false;

   if (e) {
      NETWORK_LOG(1) << BUILD_STRING("Error during HandleRead: " << e);
   }
   ASSERT(!e);


   static int i = 0;
   static int total = 0;
   static int startTime = 0;

   if (!startTime) {
      startTime = timeGetTime();
   }
   readBuf_.grow(static_cast<int>(bytes_transferred));
   ASSERT(readBuf_.size() <= ReadBufferSize);
   total += static_cast<int>(bytes_transferred);

   NETWORK_LOG(3) << endpoint_ << " received " << bytes_transferred << " bytes (read buf size is now: " << readBuf_.size() << ")";

   int deltaTime = timeGetTime() - startTime;
   double kbps = (total * 8 / 1024.0) * 1000 / deltaTime;
   NETWORK_LOG(7) << endpoint_ << " (" << std::fixed << std::setw(11) << std::setprecision(2) << kbps << ") kbps... " << total << " bytes in " << deltaTime / 1000.0 << " seconds";
   Read();
}


std::string protocol::Encode(google::protobuf::MessageLite const &msg)
{
   std::string buf;
   
   {
      // The buffer object must outlive the encoder object so the encoder
      // destructor can set the proper size.  This sounds like a bug in
      // protobufs (or maybe an unanticipated interaction between C++11
      // move semantics and protobuf)...  Hence, this scope.

      google::protobuf::io::StringOutputStream output(&buf);
      google::protobuf::io::CodedOutputStream encoder(&output);

      // Excerpt from https://developers.google.com/protocol-buffers/docs/techniques ...
      //
      // If you want to write multiple messages to a single file or stream, it
      // is up to you to keep track of where one message ends and the next begins.
      // The Protocol Buffer wire format is not self-delimiting, so protocol buffer
      // parsers cannot determine where a message ends on their own. The easiest
      // way to solve this problem is to write the size of each message before
      // you write the message itself.

      encoder.WriteLittleEndian32(msg.ByteSize());
      msg.SerializeToCodedStream(&encoder);
   }
   return buf;
}

std::string protocol::describe(const google::protobuf::Message &msg)
{
	google::protobuf::TextFormat::Printer p;
	std::string s;
   
	p.SetSingleLineMode(false);
	p.SetUseShortRepeatedPrimitives(true);
	p.PrintToString(msg, &s);

	return s;
}
