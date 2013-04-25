#ifndef _RADIANT_CLIENT_COMMAND_H
#define _RADIANT_CLIENT_COMMAND_H

BEGIN_RADIANT_CLIENT_NAMESPACE

class Command
{
public:
   virtual void operator()(void) = 0;
   virtual void Cancel() { }
};

typedef std::shared_ptr<Command> CommandPtr;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_COMMAND_H
