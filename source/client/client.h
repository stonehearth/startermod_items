#ifndef _RADIANT_CLIENT_CLIENT_H
#define _RADIANT_CLIENT_CLIENT_H

#include <boost/asio.hpp>
#include <unordered_map>
#include "protocol.h"
#include "tesseract.pb.h"
#include "metrics.h"
#include "om/om.h"
#include "dm/dm.h"
#include "dm/store.h"
#include "dm/trace_map.h"
#include "physics/octtree.h"
#include "libjson.h"
#include "core/singleton.h"
#include "selectors/selector.h"
#include "entity_traces.h"
#include "rest_api.h"
#include "chromium/chromium.h"

using boost::asio::ip::tcp;
namespace boost {
   namespace program_options {
      class options_description;
   }
}

//class ScaleformGFx;

namespace radiant {
   namespace client {
      class RestAPI;
      class InputEvent;
      class Selector;
      class RestAPI;
      class Command;

      class Client : public core::Singleton<Client> {
         public:
            Client();
            ~Client();
               
         public:
            void GetConfigOptions(boost::program_options::options_description& options);

            void run();
            int Now() { return now_; }

            RestAPI& GetAPI() { return *api_; }

            typedef std::function<void(om::EntityPtr)> SelectedTraceFn;

            dm::Guard TraceSelection(SelectedTraceFn fn);
            dm::Guard TraceDynamicObjectAlloc(std::function<void(dm::ObjectPtr)> fn) { return store_.TraceDynamicObjectAlloc(fn); }

            om::EntityPtr GetEntity(om::EntityId id);
            om::TerrainPtr GetTerrain();

            void SelectEntity(om::EntityPtr obj);
            void SelectEntity(dm::ObjectId id);

            void SelectGroundArea(Selector::SelectionFn cb);

            void ShowBuildPlan(bool visible);
            bool GetShowBuildOrders() const { return showBuildOrders_; }
            dm::Guard TraceShowBuildOrders(std::function<void()> cb);
            void EnumEntities(std::function<void(om::EntityPtr)> cb);

            void QueueEvent(std::string type, JSONNode payload);

            dm::Store& GetStore() { return store_; }
            dm::Store& GetAuthoringStore() { return authoringStore_; }
            Physics::OctTree& GetOctTree() const { return *octtree_; }

            void SetCommandCursor(std::string name);

            // For command executors only
         public:
            typedef std::function<void(const tesseract::protocol::DoActionReply*)> CommandResponseFn;
            int CreatePendingCommandResponse();
            int CreateCommandResponse(CommandResponseFn fn);
            void SendCommand(om::EntityId to, std::string action, const std::vector<om::Selection>& args, int responseId = 0);

         protected:
            typedef std::function<void()>  CommandFn;
            typedef std::function<void(std::vector<om::Selection>)> CommandMapperFn;

            void ProcessReadQueue();
            bool ProcessMessage(const tesseract::protocol::Update& msg);
            bool ProcessRequestReply(const tesseract::protocol::Update& msg);
            void BeginUpdate(const tesseract::protocol::BeginUpdate& msg);
            void EndUpdate(const tesseract::protocol::EndUpdate& msg);
            void SetServerTick(const tesseract::protocol::SetServerTick& msg);
            void DoActionReply(const tesseract::protocol::DoActionReply& msg);
            void AllocObjects(const tesseract::protocol::AllocObjects& msg);
            void UpdateObject(const tesseract::protocol::UpdateObject& msg);
            void RemoveObjects(const tesseract::protocol::RemoveObjects& msg);
            void UpdateDebugShapes(const tesseract::protocol::UpdateDebugShapes& msg);

            void mainloop();
            void setup_connections();
            void process_messages();
            void update_interpolation(int time);
            void handle_connect(const boost::system::error_code& e);
            void OnMouseInput(const MouseEvent &me, bool &handled, bool &uninstall);
            void OnKeyboardInput(const KeyboardEvent &e, bool &handled, bool &uninstall);
            void activate_comamnd(int k);

            void clearSelection();
            void hilightSelection(bool value);
            void addToSelection(om::EntityId id);
            void setSelection(const om::Selection &s);

            bool CreateStockpile(bool install);
            bool AddVoxelToPlan(bool install);
            bool MoveSelectedActor(bool install);

            void CancelPendingCommand();
            void FinishCurrentCommand();
            bool HarvestCommand(bool install);
            bool PromoteItemCommand(bool install);
            void EvalCommand(std::string cmd);

            void SendHarvestCommand(const om::Selection &s);
            void SendPromoteItemCommand(om::EntityId itemId, const om::Selection &s);
            void SendCreateStockpile(const om::Selection &s);
            void SendAddVoxelToPlan(const om::Selection &s);
            void SendMoveSelectedActor(const om::Selection &s);
            void SendBuildWorkshop(const om::Selection &s, om::EntityId id);

            bool RotateViewMode(bool install);
            bool ToggleBuildPlan();
            bool CreateFloor(bool install);
            bool GrowWalls(bool install);
            bool AddFixture(bool install, std::string fixtureName);
            bool FinishStructure(bool install);
            bool PlaceFixture(bool install, om::EntityId fixture);
            bool CapStorey(bool install);
            void AddToFloor(const om::Selection &s);
            void AssignWorkerToBuilding(std::vector<om::Selection> args);

            void Reset();
            void UpdateSelection(const MouseEvent &mouse);
            void CenterMap(const MouseEvent &mouse);

            typedef std::function<void (const tesseract::protocol::Reply&)> ReplyFn;
            typedef std::deque<std::pair<unsigned int, ReplyFn>> ReplyQueue;

            template <class T> T* GetSelected()
            {
               if (!selectedObject_) {
                  return NULL;
               }
               return selectedObject_->GetInterface<T>();
            }

            void OnObjectLoad(std::vector<om::EntityId> objects); 


            void RegisterReplyHandler(const tesseract::protocol::Command* cmd, ReplyFn fn);

#define ADD_COMMAND_TO_QUEUE(Which) AddCommandToQueue<tesseract::protocol::Which ## Command>(tesseract::protocol::Command::Which)
#define ADD_COMMAND_TO_QUEUE_EX(Which, fn) AddCommandToQueueEx<tesseract::protocol::Which ## Command>(tesseract::protocol::Command::Which, fn)

            template <class T> T* AddCommandToQueue(tesseract::protocol::Command::Type type) {
               tesseract::protocol::Command* command = _command_list.add_commands();
               command->set_id(nextCmdId_++);
               command->set_type(type);
               T* extension = command->MutableExtension(T::extension);
               return extension;
            }
            template <class T> T* AddCommandToQueueEx(tesseract::protocol::Command::Type type, ReplyFn fn) {
               tesseract::protocol::Command* command = _command_list.add_commands();
               command->set_id(nextCmdId_++);
               command->set_type(type);
               RegisterReplyHandler(command, fn);
               T* extension = command->MutableExtension(T::extension);
               return extension;
            }

            void InstallCursor();
            void SetUICursor(HCURSOR cursor);
            void ExecuteCommands();
            void SetRenderPipelineInfo();
            void OnDestroyed(dm::ObjectId id);
            void OnAuthoringObjectAllocated(dm::ObjectPtr obj);
            void TraceEntity(PendingCommandPtr cmd);
            void TraceEntities(PendingCommandPtr cmd);
            void FetchJsonData(PendingCommandPtr cmd);
            void SetViewModeCommand(PendingCommandPtr cmd);
            void SelectEntityCommand(PendingCommandPtr cmd);
            void SelectToolCommand(PendingCommandPtr cmd);
            void HilightMouseover();
            void LoadCursors();
            void OnEntityAlloc(om::EntityPtr entity);
            void ComponentAdded(om::EntityRef e, dm::ObjectType type, std::shared_ptr<dm::Object> component);
            void BrowserRequestHandler(std::string const& uri, std::string const& query, std::string const& postdata, std::shared_ptr<chromium::IResponse> response) const;

      private:
            typedef std::unordered_map<dm::TraceId, SelectedTraceFn> SelectedTraceMap;
            enum TraceTypes {
               ShowBuildOrdersTraces,
            };

            class CursorDeleter {
            public:
               CursorDeleter() { };
               
               // The pointer typedef declares the type to be returned from std::unique_ptr<>::get.
               // This would default to HCURSOR* without this typedef.
               typedef HCURSOR pointer;

               void operator()(pointer cursor) {
                  DestroyCursor(cursor);
               }
            };

      protected:
            boost::asio::io_service _io_service;
            tcp::socket             _tcp_socket;

            dm::TraceId             nextTraceId_;
            dm::GuardSet              guards_;
            SelectedTraceMap        selectedTraces_;

            int                     _send_ahead_commands;

            // xxx: move this to the proxy manager
            dm::Store                        store_;
            dm::Store                        authoringStore_;
            om::EntityPtr                    rootObject_;
            //std::unique_ptr<ScaleformGFx>         _scaleform;
            std::unique_ptr<Physics::OctTree>     octtree_;
            std::unique_ptr<chromium::IBrowser>   browser_;

            uint32                           _server_last_update_time;
            uint32                           _server_interval_duration;
            uint32                           _client_interval_start;

            //map<int, std::shared_ptr<command>>    _commands;
            //std::shared_ptr<command>              _current_command;

            std::map<int, CommandFn>            _commands;
            std::map<std::string, CommandMapperFn>   _mappedCommands;

            om::EntityPtr                    selectedObject_;
            std::vector<om::EntityRef>       hilightedObjects_;
            tesseract::protocol::command_list        _command_list;
            protocol::SendQueuePtr           send_queue_;
            protocol::RecvQueuePtr           recv_queue_;

            int                              last_sequence_number_;
            unsigned int                     nextCmdId_;
            ReplyQueue                       replyQueue_;
            HCURSOR                          uiCursor_;
            HCURSOR                          currentCursor_;
            HCURSOR                          defaultCursor_;
            HWND                             hwnd_;
            std::shared_ptr<Selector>             selector_;
            int                              now_;
            std::unique_ptr<RestAPI>         api_;
            int                              nextDeferredCommandId_;
            bool                             showBuildOrders_;

            std::unordered_map<dm::ObjectId, std::shared_ptr<dm::Object>> objects_;
            std::unordered_map<dm::ObjectId, om::EntityPtr> entities_;
            dm::TraceMap<TraceTypes>         traces_;
            std::map<std::pair<RestAPI::SessionId, dm::TraceId>, std::shared_ptr<TraceInterface>> _entitiesTraces;
            std::shared_ptr<Command>         currentCommand_;
            HCURSOR                          currentCommandCursor_;
            std::shared_ptr<Command>         currentTool_;
            std::string                      currentToolName_;

            int                              nextResponseHandlerId_;
            std::map<int, CommandResponseFn> responseHandlers_;
            std::unordered_map<std::string, std::unique_ptr<HCURSOR, Client::CursorDeleter>> cursors_;
            std::vector<om::EntityRef>       alloced_;

            int                              last_server_request_id_;
            std::map<int, std::function<void(tesseract::protocol::Update const& reply)> >  server_requests_;
      };
   };
};

#endif // _RADIANT_CLIENT_CLIENT_H
