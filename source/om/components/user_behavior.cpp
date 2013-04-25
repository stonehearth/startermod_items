#include "pch.h"
#include "user_behavior.h"

using namespace ::radiant;
using namespace ::radiant::om;

void UserBehavior::InitializeRecordFields()
{
   AddRecordField("script", scriptName_);
   AddRecordField("config", jsonConfig_);
}

void UserBehavior::SetScriptName(std::string script)
{
   scriptName_ = script;
}

void UserBehavior::SetConfigString(std::string config)
{
   jsonConfig_ = config;
}

std::string UserBehavior::GetScriptName() const
{
   return *scriptName_;
}

std::string UserBehavior::GetConfigString() const
{
   return *jsonConfig_;
}

luabind::object UserBehavior::GetScriptInstance() const
{
   return scriptInstance_;
}

void UserBehavior::SetScriptInstance(luabind::object instance)
{
   scriptInstance_ = instance;
}
