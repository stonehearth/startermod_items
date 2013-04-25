#ifndef _RADIANT_CLIENT_PENDING_COMMAND_H
#define _RADIANT_CLIENT_PENDING_COMMAND_H

#include "namespace.h"
#include "response.h"

#define CHECK_TYPE(response, args, key, expectedType) \
   do { \
      auto i = args.find(key); \
      if (i == args.end()) { \
         response->Error("missing argument " #key "."); \
         return; \
      } \
      if (i->type() != expectedType) { \
         response->Error("wrong type for argument " #key "."); \
         return; \
      } \
   } while (false)

BEGIN_RADIANT_CLIENT_NAMESPACE

class PendingCommand
{
public:
   PendingCommand(const JSONNode& cmd, ResponsePtr response);

   int GetSession() const;
   void SetSession(int session);

   std::string GetCommand() const;
   const JSONNode& GetArgs() const;

   void Complete(JSONNode result);
   void Defer(int id);
   void Error(std::string result);

private:
   JSONNode       cmd_;
   ResponsePtr    response_;
};

typedef std::shared_ptr<PendingCommand> PendingCommandPtr;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_PENDING_COMMAND_H
