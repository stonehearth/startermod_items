#include "radiant.h"
#include <iostream>
#include <iomanip> 
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include "protocol.h"

using namespace ::radiant;
using namespace ::radiant::protocol;

SendQueuePtr SendQueue::Create(boost::asio::ip::tcp::socket& s)
{
   return std::make_shared<SendQueue>(s);
}

void SendQueue::Flush(std::shared_ptr<SendQueue> sendQueue)
{
   ASSERT(sendQueue);
   
   if (sendQueue) {
      auto &queue = sendQueue->queue_;

      while (!queue.empty()) {
         auto buffer = queue.front();
         sendQueue->socket_.async_send(boost::asio::buffer(buffer->data(), buffer->size()), 
                                       std::bind(&SendQueue::HandleWrite, sendQueue, buffer, std::placeholders::_1, std::placeholders::_2));
         queue.pop();
      }
   }
}

void SendQueue::Push(google::protobuf::MessageLite const& protobuf)
{
   Push(Encode(protobuf));
}

void SendQueue::Push(std::string&& s)
{
   queue_.push(std::make_shared<std::string>(std::forward<std::string>(s)));
}

void SendQueue::HandleWrite(SendQueuePtr queue, std::shared_ptr<std::string> buffer, const boost::system::error_code& error, size_t bytes_transferred)
{
   ASSERT(!error);
}

RecvQueue::RecvQueue(boost::asio::ip::tcp::socket& s) : 
   socket_(s),
   readPending_(false),
   readBuf_(ReadBufferSize)
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
      LOG(WARNING) << "buffer full in RecvQueue::Read.  Waiting for next process loop...";
   }
}

void RecvQueue::HandleRead(RecvQueuePtr q, const boost::system::error_code& e, std::size_t bytes_transferred)
{
   ASSERT(readPending_); 
   readPending_ = false;

   std::string error = e.message();
   if (!e) {
      static int i = 0;
      static int total = 0;
      static int startTime = 0;

      if (!startTime) {
         startTime = timeGetTime();
      }
      readBuf_.grow(bytes_transferred);
      total += bytes_transferred;

      int deltaTime = timeGetTime() - startTime;
      double kbps = (total * 8 / 1024.0) * 1000 / deltaTime;
      LOG(INFO) << "(" << std::fixed << std::setw(11) << std::setprecision(2) << kbps << ") kbps... " << total << " bytes in " << deltaTime / 1000.0 << " seconds";
      Read();
   }
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
