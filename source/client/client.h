#ifndef _RADIANT_CLIENT_CLIENT_H
#define _RADIANT_CLIENT_CLIENT_H

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <unordered_map>
#include <mutex>
#include "namespace.h"
#include "radiant_net.h"
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
#include "chromium/chromium.h"
#include "lua/namespace.h"
#include "mouse_event_promise.h"
#include "trace_object_deferred.h"
#include "radiant_json.h"

IN_RADIANT_LUA_NAMESPACE(
   class ScriptHost;
)

BEGIN_RADIANT_CLIENT_NAMESPACE

class InputEvent;
class Command;

void RegisterLuaTypes(lua_State* L);

class Client : public core::Singleton<Client> {
   public:
      Client();
      ~Client();

      static void RegisterLuaTypes(lua_State* L);
            
   public: // xxx: just for lua...
      void BrowserRequestHandler(std::string const& uri, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response);
      TraceObjectDeferredPtr TraceObject(std::string const& uri, const char* reason);

   public:
      void GetConfigOptions(boost::program_options::options_description& options);

      void run();
      lua::ScriptHost* GetScriptHost() const { return scriptHost_.get(); }
            
      om::EntityPtr GetEntity(dm::ObjectId id);
      om::TerrainPtr GetTerrain();

      void SelectEntity(om::EntityPtr obj);
      void SelectEntity(dm::ObjectId id);

      om::EntityPtr CreateAuthoringEntity(std::string const& mod_name, std::string const& entity_name);

      dm::Store& GetStore() { return store_; }
      dm::Store& GetAuthoringStore() { return authoringStore_; }
      Physics::OctTree& GetOctTree() const { return *octtree_; }

      void SetCursor(std::string name);            

   private:
      typedef std::function<void()>  CommandFn;
      typedef std::function<void(std::vector<om::Selection>)> CommandMapperFn;

      void QueueEvent(std::string type, JSONNode payload);
      void ProcessReadQueue();
      bool ProcessMessage(const tesseract::protocol::Update& msg);
      bool ProcessRequestReply(const tesseract::protocol::Update& msg);
      void BeginUpdate(const tesseract::protocol::BeginUpdate& msg);
      void EndUpdate(const tesseract::protocol::EndUpdate& msg);
      void SetServerTick(const tesseract::protocol::SetServerTick& msg);
      void AllocObjects(const tesseract::protocol::AllocObjects& msg);
      void UpdateObject(const tesseract::protocol::UpdateObject& msg);
      void RemoveObjects(const tesseract::protocol::RemoveObjects& msg);
      void UpdateDebugShapes(const tesseract::protocol::UpdateDebugShapes& msg);
      void DefineRemoteObject(const tesseract::protocol::DefineRemoteObject& msg);

      void mainloop();
      void setup_connections();
      void process_messages();
      void update_interpolation(int time);
      void handle_connect(const boost::system::error_code& e);
      void OnMouseInput(const MouseEvent &windowMouse, const MouseEvent &browserMouse, bool &handled, bool &uninstall);
      void OnKeyboardInput(const KeyboardEvent &e, bool &handled, bool &uninstall);

      void Reset();
      void UpdateSelection(const MouseEvent &mouse);
      void CenterMap(const MouseEvent &mouse);

      void EvalCommand(std::string cmd);
      void InstallCursor();
      void HilightMouseover();
      void LoadCursors();
      void GetEvents(JSONNode const& query, std::shared_ptr<net::IResponse> response);
      void HandlePostRequest(std::string const& path, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response);
      void HandleClientRouteRequest(luabind::object ctor, JSONNode const& query, std::string const& postdata, std::shared_ptr<net::IResponse> response);
      void TraceUri(JSONNode const& query, std::shared_ptr<net::IResponse> response);
      void GetModules(JSONNode const& query, std::shared_ptr<net::IResponse> response);
      bool TraceObjectUri(std::string const& uri, std::shared_ptr<net::IResponse> response);
      void TraceFileUri(std::string const& uri, std::shared_ptr<net::IResponse> response);
      void FlushEvents();
      void DestroyAuthoringEntity(dm::ObjectId id);
      MouseEventPromisePtr TraceMouseEvents();
      void LoadModuleInitScript(json::ConstJsonObject const& block);
      void LoadModuleRoutes(std::string const& modulename, json::ConstJsonObject const& block);

      typedef std::function<void(tesseract::protocol::Update const& msg)> ServerReplyCb;
      void PushServerRequest(tesseract::protocol::Request& msg, ServerReplyCb replyCb);
      void AddBrowserJob(std::function<void()> fn);
      void ProcessBrowserJobQueue();

private:
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

private:
      // connection to the server...
      boost::asio::io_service       _io_service;
      boost::asio::ip::tcp::socket  _tcp_socket;
      uint32                        _server_last_update_time;
      uint32                        _server_interval_duration;
      uint32                        _client_interval_start;
      int                              last_sequence_number_;
      protocol::SendQueuePtr           send_queue_;
      protocol::RecvQueuePtr           recv_queue_;
      int                              now_;

      // the local object trace system...
      dm::TraceId             nextTraceId_;
      std::map<dm::TraceId, dm::Guard>         uriTraces_;
   
      // remote object storage and tracking...
      dm::Store                        store_;
      om::EntityPtr                    rootObject_;
      om::EntityPtr                    selectedObject_;
      std::vector<om::EntityRef>       hilightedObjects_;
      std::unordered_map<dm::ObjectId, std::shared_ptr<dm::Object>> objects_;

      // local authoring object storage and tracking...
      dm::Store                        authoringStore_;
      std::unordered_map<dm::ObjectId, om::EntityPtr> authoredEntities_;

      // local collision tests...
      std::unique_ptr<Physics::OctTree>     octtree_;

      // the ui browser object...
      std::unique_ptr<chromium::IBrowser>   browser_;
      std::vector<JSONNode>            queued_events_;
      std::shared_ptr<net::IResponse>   get_events_request_;
      std::unordered_map<std::string, luabind::object>   clientRoutes_;
      std::vector<std::function<void()>>     browserJobQueue_;
      std::mutex                             browserJobQueueLock_;

      // client side command dispatching...
      std::map<int, CommandFn>            _commands;

      // cursors...
      HCURSOR                          currentCursor_;
      HCURSOR                          luaCursor_;
      HCURSOR                          uiCursor_;
      std::unordered_map<std::string, std::unique_ptr<HCURSOR, Client::CursorDeleter>> cursors_;

      // server requests...
      int                              last_server_request_id_;
      std::map<int, std::function<void(tesseract::protocol::Update const& reply)> >  server_requests_;

      // server side remote object tracking...
      std::unordered_map<std::string, std::string>    serverRemoteObjects_;
      std::unordered_map<std::string, std::string>    clientRemoteObjects_;
      std::unordered_map<std::string, TraceObjectDeferredRef>  deferredObjectTraces_;
      om::ErrorBrowserPtr                             error_browser_;

      // client side lua...
      std::unique_ptr<lua::ScriptHost>  scriptHost_;
      std::vector<MouseEventPromiseRef>   mouseEventPromises_;

};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_CLIENT_H
