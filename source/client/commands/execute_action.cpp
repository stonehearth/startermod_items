#include "pch.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include "execute_action.h"
#include "client/client.h"
#include "resources/res_manager.h"
#include "resources/action.h"
#include "om/selection.h"

using namespace radiant;
using namespace radiant::client;

ExecuteAction::ExecuteAction(PendingCommandPtr cmd)
{
   // xxx: just use a schema validator for this.  BEFORE it gets to the command!
   args_ = cmd->GetArgs();
   CHECK_TYPE(cmd, args_, "action", JSON_STRING);
   
   om::EntityId self = 0;
   auto i = args_.find("self");
   if (i != args_.end()) {
      self = i->as_int();
      if (!Client::GetInstance().GetEntity(self)) {
         cmd->Error("Entity " + stdutil::ToString(self) + " does not exist.");
         return;
      }
   }

   std::string actionName = args_["action"].as_string();
   auto action = resources::ResourceManager2::GetInstance().Lookup<resources::Action>(actionName);
   if (!action) {
      cmd->Error("No such action " + actionName);
      return;
   }

   self_ = self;
   action_ = action;

   deferredCommandId_ = Client::GetInstance().CreatePendingCommandResponse();
   cmd->Defer(deferredCommandId_);
}

ExecuteAction::~ExecuteAction()
{
   if (selector_) {
      selector_->Deactivate();
   }
   Client::GetInstance().SetCommandCursor("");
}


void ExecuteAction::operator()()
{
   // xxx - this is annoying... if the initialization code fails, we shouldn't
   // even get this far, no?
   if (!action_) {
      return;
   }
   const JSONNode& formal = action_->GetArgs();
   Client::GetInstance().SetCommandCursor(action_->GetCursor());

   for (auto i = actual_.size(); i < formal.size(); i++) {

      std::string type = formal[i].as_string();

      if (type == "self") {
         om::Selection arg;
         arg.AddEntity(self_);
         actual_.push_back(arg);
      } else if (type == "ground_area") {
         auto cb = [=](const om::Selection &area) {
            actual_.push_back(area);
            (*this)();
         };
         auto selector = std::make_shared<VoxelRangeSelector>(Client::GetInstance().GetTerrain());
         selector->RegisterSelectionFn(cb);
         selector->SetColor(math3d::color3(192, 192, 192));
         selector->SetDimensions(2);
         selector->Activate();
         selector_ = selector;
      } else if (type == "entity") {
         auto cb = [=](const om::Selection &area) {
            actual_.push_back(area);
            (*this)();
         };
         selector_ = std::make_shared<ActorSelector>(cb);
      } else if (boost::starts_with(type, "{{") && boost::ends_with(type, "}}")) {
         std::string key(type.begin() + 2, type.end() - 2);
         auto i = args_.find(key);
         if (i != args_.end()) {
            switch (i->type()) {
            case JSON_ARRAY:
            case JSON_NODE:
               actual_.push_back(om::Selection(i->write()));
               break;
            case JSON_NUMBER:
               actual_.push_back(om::Selection(i->as_int()));
               break;
            case JSON_STRING:
               actual_.push_back(om::Selection(i->as_string()));
               break;
            case JSON_BOOL:
               actual_.push_back(om::Selection(i->as_bool() ? 1 : 0));
               break;
            default:
               actual_.push_back(om::Selection(""));
            }
         } else {
            actual_.push_back(om::Selection("{}"));
         }
      }
#if 0
   } else if (type == "select-actor") {
      _pendingCommand = std::bind(&Client::SetArg_Actor, this, cmd, self, args, _1);
   } else if (type == "ground-area") {
      _pendingCommand = std::bind(&Client::SetArg_GroundArea, this, cmd, self, args, _1);
   } else if (type == "ground-location") {
      _pendingCommand = std::bind(&Client::SetArg_GroundLocation, this, cmd, self, args, _1);
   } else {
      _pendingCommand = std::bind(&Client::SetArg_String, this, type, args);
#endif
   }
   if (formal.size() == actual_.size()) {
      std::string cmd = action_->GetCommand();
      Client::GetInstance().SendCommand(self_, cmd, actual_, deferredCommandId_);
   }
}

#if 0
bool Client::SetArg_String(std::string arg, std::shared_ptr<vector<selection>> args)
{
   selection s;
   s.str = arg;
   args->push_back(s);
   return true;
}

bool Client::SetArg_Actor(const ObjectModel::ICommand* cmd, const om::EntityPtr self, std::shared_ptr<vector<selection>> args, bool install)
{
#if 1
   if (install) {
      auto cb = std::bind(&Client::SetArg_GroundAreaCb, this, cmd, self, args, _1);
      auto vrs = std::make_shared<ActorSelector>(cb);
      selector_ = vrs;
   }
   return false;
#else
   if (install) {
      // xxx: pass the register fn into ActivateActorSelector and other activation fns
      render3d::ActorSelector *selector = _renderer->ActivateActorSelector();
      selector->RegisterSelectionFn(std::bind(&Client::SetArg_GroundAreaCb, this, cmd, self, args, _1));
   } else {
      _renderer->DeactivateSelector();
   }
#endif
   return false;
}

bool Client::SetArg_GroundArea(const ObjectModel::ICommand* cmd, const om::EntityPtr self, std::shared_ptr<vector<selection>> args, bool install)
{
   if (install) {
      auto cb = std::bind(&Client::SetArg_GroundAreaCb, this, cmd, self, args, _1);
      auto vrs = std::make_shared<VoxelRangeSelector>(GetTerrain(), cb);
      vrs->SetColor(math3d::color3(192, 192, 192));
      vrs->SetDimensions(2);
      selector_ = vrs;
   }
   return false;
}
               
bool Client::SetArg_GroundLocation(const ObjectModel::ICommand* cmd, const om::EntityPtr self, std::shared_ptr<vector<selection>> args, bool install)
{
#if 1
   assert(false);
#else
   if (install) {
      render3d::PlaceModelSelector *selector = _renderer->ActivatePlaceModelSelector();
      selector->RegisterSelectionFn(std::bind(&Client::SetArg_GroundAreaCb, this, cmd, self, args, _1));
      //selector->SetGrid(GetTerrain().GetGrid());
      selector->SetModel(cmd->GetCursor());
      selector->SetBounds(math3d::ibounds3(math3d::ipoint3(0, 0, 0), math3d::ipoint3(8, 5, -8))); // xxx: wtf??
   } else {
      _renderer->DeactivateSelector();
   }
#endif
   return false;
}

void Client::SetArg_GroundAreaCb(const ObjectModel::ICommand* cmd, const om::EntityPtr self, std::shared_ptr<vector<selection>> args, const selection &result)
{
   args->push_back(result);
   _pendingCommand = std::bind(&Client::CollectArgs, this, cmd, self, args, _1);
   _pendingCommand(true);
}
#endif
