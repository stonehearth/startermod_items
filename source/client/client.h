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
#include "radiant_json.h"
#include "lib/rpc/forward_defines.h"
#include "core/input.h"
#include "core/unique_resource.h"

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
            
   public:
      void GetConfigOptions(boost::program_options::options_description& options);

      void run();
      lua::ScriptHost* GetScriptHost() const { return scriptHost_.get(); }
      void BrowserRequestHandler(std::string const& uri, json::ConstJsonObject const& query, std::string const& postdata, rpc::HttpDeferredPtr response);
            
      om::EntityPtr GetEntity(dm::ObjectId id);
      om::TerrainPtr GetTerrain();

      void SelectEntity(om::EntityPtr obj);
      void SelectEntity(dm::ObjectId id);

      om::EntityPtr CreateEmptyAuthoringEntity();
      om::EntityPtr CreateAuthoringEntity(std::string const& mod_name, std::string const& entity_name);
      om::EntityPtr CreateAuthoringEntityByRef(std::string const& ref);
      void DestroyAuthoringEntity(dm::ObjectId id);

      dm::Store& GetStore() { return store_; }
      dm::Store& GetAuthoringStore() { return authoringStore_; }
      Physics::OctTree& GetOctTree() const { return *octtree_; }

      void SetCursor(std::string name);

      typedef int InputHandlerId;
      typedef std::function<int(Input const&)> InputHandlerCb;

      InputHandlerId AddInputHandler(InputHandlerCb const& cb);
      InputHandlerId ReserveInputHandler();
      void SetInputHandler(InputHandlerId id, InputHandlerCb const& cb);
      void RemoveInputHandler(InputHandlerId id);

      typedef int TraceRenderFrameId;
      typedef std::function<int(float)> TraceRenderFrameHandlerCb;

      TraceRenderFrameId AddTraceRenderFrameHandler(TraceRenderFrameHandlerCb const& cb);
      TraceRenderFrameId ReserveTraceRenderFrameHandler();
      void SetTraceRenderFrameHandler(TraceRenderFrameId id, TraceRenderFrameHandlerCb const& cb);
      void RemoveTraceRenderFrameHandler(TraceRenderFrameId id);
      bool CallTraceRenderFrameHandlers(float frameTime);

   private:
      NO_COPY_CONSTRUCTOR(Client);

      typedef std::function<void()>  CommandFn;
      typedef std::function<void(std::vector<om::Selection>)> CommandMapperFn;

      void ProcessReadQueue();
      bool ProcessMessage(const tesseract::protocol::Update& msg);
      bool ProcessRequestReply(const tesseract::protocol::Update& msg);
      void PostCommandReply(const tesseract::protocol::PostCommandReply& msg);
      void BeginUpdate(const tesseract::protocol::BeginUpdate& msg);
      void EndUpdate(const tesseract::protocol::EndUpdate& msg);
      void SetServerTick(const tesseract::protocol::SetServerTick& msg);
      void AllocObjects(const tesseract::protocol::AllocObjects& msg);
      void UpdateObject(const tesseract::protocol::UpdateObject& msg);
      void RemoveObjects(const tesseract::protocol::RemoveObjects& msg);
      void UpdateDebugShapes(const tesseract::protocol::UpdateDebugShapes& msg);
      void DefineRemoteObject(const tesseract::protocol::DefineRemoteObject& msg);

      void mainloop();
      void InitializeModules();
      void setup_connections();
      void process_messages();
      void update_interpolation(int time);
      void handle_connect(const boost::system::error_code& e);
      void OnInput(Input const& input);
      void OnMouseInput(Input const& mouse);
      void OnKeyboardInput(Input const& keyboard);
      void OnRawInput(Input const& keyboard);
      bool CallInputHandlers(Input const& input);

      void Reset();
      void UpdateSelection(const MouseInput &mouse);
      void CenterMap(const MouseInput &mouse);

      void InstallCursor();
      void HilightMouseover();
      void LoadCursors();
      rpc::ReactorDeferredPtr GetModules(rpc::Function const&);
      void HandlePostRequest(std::string const& path, JSONNode const& query, std::string const& postdata, rpc::HttpDeferredPtr response);
      void TraceUri(JSONNode const& query, rpc::HttpDeferredPtr response);
      bool TraceObjectUri(std::string const& uri, rpc::HttpDeferredPtr response);
      void TraceFileUri(std::string const& uri, rpc::HttpDeferredPtr response);
      void LoadModuleInitScript(json::ConstJsonObject const& block);
      void LoadModuleRoutes(std::string const& modulename, json::ConstJsonObject const& block);

      typedef std::function<void(tesseract::protocol::Update const& msg)> ServerReplyCb;
      void PushServerRequest(tesseract::protocol::Request& msg, ServerReplyCb replyCb);
      void AddBrowserJob(std::function<void()> fn);
      void HandleCallRequest(json::ConstJsonObject const& node, rpc::HttpDeferredPtr response);
      void ProcessBrowserJobQueue();
      void HandleServerCallRequest(std::string const& obj, std::string const& function_name, json::ConstJsonObject const& node, rpc::HttpDeferredPtr response);
      void BrowserCallRequestHandler(json::ConstJsonObject const& query, std::string const& postdata, rpc::HttpDeferredPtr response);
      void CallHttpReactor(std::string parts, json::ConstJsonObject query, std::string postdata, rpc::HttpDeferredPtr response);

private:
      /*
       * The type of DestroyCursor is WINUSERAPI BOOL WINAPI (HCURSOR).  Strip off all
       * that windows fu so we can pass it into the Deleter for a core::UniqueResource
       */
      static inline void CursorDeleter(HCURSOR hcursor) { DestroyCursor(hcursor); }
      typedef std::unordered_map<std::string, core::UniqueResource<HCURSOR, Client::CursorDeleter>> CursorMap;

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
      std::unordered_map<std::string, luabind::object>   clientRoutes_;
      std::vector<std::function<void()>>     browserJobQueue_;
      std::mutex                             browserJobQueueLock_;

      // client side command dispatching...
      std::map<int, CommandFn>            _commands;

      // cursors...
      HCURSOR                          currentCursor_;
      HCURSOR                          luaCursor_;
      HCURSOR                          uiCursor_;
      CursorMap                        cursors_;

      // server requests...
      int                              last_server_request_id_;
      std::map<int, std::function<void(tesseract::protocol::Update const& reply)> >  server_requests_;

      // server side remote object tracking...
      om::ErrorBrowserPtr                             error_browser_;

      // client side lua...
      std::unique_ptr<lua::ScriptHost>  scriptHost_;

      InputHandlerId                                           next_input_id_;
      std::vector<std::pair<InputHandlerId, InputHandlerCb>>   input_handlers_;

      TraceRenderFrameId                                                      next_trace_frame_id_;
      std::vector<std::pair<TraceRenderFrameId, TraceRenderFrameHandlerCb>>   trace_frame_handlers_;

      // reactor...
      rpc::CoreReactorPtr         core_reactor_;
      rpc::HttpReactorPtr         http_reactor_;
      rpc::HttpDeferredPtr        get_events_deferred_;
      rpc::ProtobufRouterPtr      protobuf_router_;
      int                         mouse_x_;
      int                         mouse_y_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_CLIENT_H
