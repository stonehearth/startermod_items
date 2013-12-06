#ifndef _RADIANT_PROTOCOL_H
#define _RADIANT_PROTOCOL_H

#include <google/protobuf/message.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <boost/asio.hpp>
#include <queue>

#define NETWORK_LOG(level)    LOG(network, level)

namespace radiant {
   namespace protocol {
      static const int ReadLowWaterMark = 1024;
      static const int ReadBufferSize   = 256 * 1024;

      class Buffer {
      public:
         Buffer(int c) : _readOffset(0), _writeOffset(0), _capacity(c) { _data = (char *)malloc(c); }
         ~Buffer() { free(_data); }

         void *data() { return (void *)(_data + _readOffset); }
         void *nextdata() { return (void *)(_data + _writeOffset); }

         int size() { return (_writeOffset - _readOffset); }
         int remaining() { return _capacity - _writeOffset; }
         void clear() { _readOffset = _writeOffset = 0; }
         bool empty() { return _readOffset == _writeOffset; }

         void compress() {
            if (_readOffset == _writeOffset) {
               // Fast compression. Just go back to the beginning!
               _readOffset = _writeOffset = 0;
            } else if (_readOffset != 0) {
               // If we've consumed into the middle of the buffer, move the valid bits
               // back to the start.  If _readOffset is 0, there's no need to do the
               // move at all.
               int c = _writeOffset - _readOffset;
               NETWORK_LOG(7) << "moving " << c << " bytes of data to compress buffer (read:" << _readOffset << " write:" << _writeOffset << ")";
               memmove(_data, _data + _readOffset, c);
               _readOffset = 0;
               _writeOffset = c;
            }
         }

         void grow(int delta) { _writeOffset += delta; }
         void consume(int delta) { _readOffset += delta; }

      public:
         char        *_data;
         int         _readOffset;
         int         _writeOffset;
         int         _capacity;
      };
      std::string describe(const google::protobuf::Message &msg);

      class SendQueue;
      class RecvQueue;

      typedef std::shared_ptr<Buffer> BufferPtr;
      typedef std::shared_ptr<SendQueue> SendQueuePtr;
      typedef std::shared_ptr<RecvQueue> RecvQueuePtr;

      class SendQueue {
      public:
         static SendQueuePtr Create(boost::asio::ip::tcp::socket& s);
         static void Flush(SendQueuePtr q);

      public:
         bool Empty() const { return queue_.empty(); }
         void Push(google::protobuf::MessageLite const &protobuf);
         void Push(std::string&& s);

      public: // xxx - only because make_shared is silly
         SendQueue(boost::asio::ip::tcp::socket& s) : socket_(s) { }
         static void HandleWrite(SendQueuePtr queue, std::shared_ptr<std::string> buffer, const boost::system::error_code& error, size_t bytes_transferred);

      private:
         boost::asio::ip::tcp::socket& socket_;
         std::queue<std::shared_ptr<std::string>> queue_;
      };

      class RecvQueue : public std::enable_shared_from_this<RecvQueue> {
      public:
         RecvQueue(boost::asio::ip::tcp::socket& s);

      public:
         template <class T> void Process(std::function<bool(T&)> fn)
         {
            google::protobuf::io::ArrayInputStream input(readBuf_.data(), readBuf_.size());
            google::protobuf::io::CodedInputStream decoder(&input);

            int remaining = readBuf_.size();

            // Read all we can...
            while (!readBuf_.empty()) {
               const void* tail;
               if (!decoder.GetDirectBufferPointer(&tail, &remaining)) {
                  remaining = 0;
                  break;
               }

               T msg;
               google::protobuf::uint32 c = 0;

               if (!decoder.ReadLittleEndian32(&c)) {
                  break;
               }
               NETWORK_LOG(7) << "next message size: " << c;
               if (c > (remaining - sizeof(int32))) {
                  NETWORK_LOG(3) << "breaking processing loop early (not enough buffer: " << (remaining - sizeof(int32)) << " < " << c << ")";
                  break;
               }
               ASSERT(c > 0 && c < 256 * 1024);

               auto limit = decoder.PushLimit(c);
               if (!msg.ParseFromCodedStream(&decoder)) {
                  break;
               }
               decoder.PopLimit(limit);
               if (!fn(msg)) {
                  break;
               }
            }
            int consumed = readBuf_.size() - remaining;
            readBuf_.consume(consumed);
            if (!readPending_) {
               Read();
            }
         }

      protected:
         void HandleRead(RecvQueuePtr q, const boost::system::error_code& e, std::size_t bytes_transferred);

      private:
         void Read();

      private:
         boost::asio::ip::tcp::socket&    socket_;
         bool                             readPending_;
         Buffer                           readBuf_;
      };

     std::string Encode(google::protobuf::MessageLite const& protobuf);
   };
};

#endif // _RADIANT_PROTOCOL_H
