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
#include "protocols/tesseract.pb.h"
#include "metrics.h"
#include "om/om.h"
#include "dm/dm.h"
#include "dm/store.h"
#include "physics/octtree.h"
#include "libjson.h"
#include "core/singleton.h"
#include "chromium/chromium.h"
#include "lib/lua/lua.h"
#include "lib/json/node.h"
#include "lib/rpc/forward_defines.h"
#include "core/input.h"
#include "core/guard.h"
#include "core/unique_resource.h"
#include "core/shared_resource.h"
//#include <SFML/Audio.hpp>

namespace sf{class Sound;}

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
      void run(int server_port);
      lua::ScriptHost* GetScriptHost() const { return scriptHost_.get(); }
      void BrowserRequestHandler(std::string const& uri, json::Node const& query, std::string const& postdata, rpc::HttpDeferredPtr response);
            
      om::EntityPtr GetEntity(dm::ObjectId id);
      om::TerrainPtr GetTerrain();

      void SelectEntity(om::EntityPtr obj);
      void SelectEntity(dm::ObjectId id);

      om::EntityPtr CreateEmptyAuthoringEntity();
      om::EntityPtr CreateAuthoringEntity(std::string const& uri);
      void DestroyAuthoringEntity(dm::ObjectId id);

      dm::Store& GetStore() { return store_; }
      dm::Store& GetAuthoringStore() { return authoringStore_; }
      phys::OctTree& GetOctTree() const { return *octtree_; }

      typedef int CursorStackId;
      CursorStackId InstallCursor(std::string name);
      void RemoveCursor(CursorStackId id);

      typedef int InputHandlerId;
      typedef std::function<bool(Input const&)> InputHandlerCb;

      InputHandlerId AddInputHandler(InputHandlerCb const& cb);
      InputHandlerId ReserveInputHandler();
      void SetInputHandler(InputHandlerId id, InputHandlerCb const& cb);
      void RemoveInputHandler(InputHandlerId id);

      typedef int TraceRenderFrameId;
      typedef std::function<void(float)> TraceRenderFrameHandlerCb;

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

      void InstallCurrentCursor();
      void HilightMouseover();

      static inline void CursorDeleter(HCURSOR hcursor) { DestroyCursor(hcursor); }
      typedef core::SharedResource<HCURSOR, Client::CursorDeleter> Cursor;

      Cursor LoadCursor(std::string const& path);

      rpc::ReactorDeferredPtr GetModules(rpc::Function const&);
      void HandlePostRequest(std::string const& path, JSONNode const& query, std::string const& postdata, rpc::HttpDeferredPtr response);
      void TraceUri(JSONNode const& query, rpc::HttpDeferredPtr response);
      bool TraceObjectUri(std::string const& uri, rpc::HttpDeferredPtr response);
      void TraceFileUri(std::string const& uri, rpc::HttpDeferredPtr response);
      void LoadModuleInitScript(json::Node const& block);
      void LoadModuleRoutes(std::string const& modulename, json::Node const& block);

      typedef std::function<void(tesseract::protocol::Update const& msg)> ServerReplyCb;
      void PushServerRequest(tesseract::protocol::Request& msg, ServerReplyCb replyCb);
      void AddBrowserJob(std::function<void()> fn);
      void HandleCallRequest(json::Node const& node, rpc::HttpDeferredPtr response);
      void ProcessBrowserJobQueue();
      void HandleServerCallRequest(std::string const& obj, std::string const& function_name, json::Node const& node, rpc::HttpDeferredPtr response);
      void BrowserCallRequestHandler(json::Node const& query, std::string const& postdata, rpc::HttpDeferredPtr response);
      void CallHttpReactor(std::string parts, json::Node query, std::string postdata, rpc::HttpDeferredPtr response);
      void InitDataModel();

private:
      /*
       * The type of DestroyCursor is WINUSERAPI BOOL WINAPI (HCURSOR).  Strip off all
       * that windows fu so we can pass it into the Deleter for a core::UniqueResource
       */
      typedef std::unordered_map<std::string, Cursor>    CursorMap;

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
      int                              _server_skew;
      int                              server_port_;

      // the local object trace system...
      dm::TraceId             nextTraceId_;
      std::map<dm::TraceId, core::Guard>         uriTraces_;
   
      // remote object storage and tracking...
      dm::Store                        store_;
      om::EntityPtr                    rootObject_;
      om::EntityRef                    selectedObject_;
      std::vector<om::EntityRef>       hilightedObjects_;

      // local authoring object storage and tracking...
      dm::Store                        authoringStore_;
      std::unordered_map<dm::ObjectId, om::EntityPtr> authoredEntities_;

      // local collision tests...
      std::unique_ptr<phys::OctTree>     octtree_;

      // the ui browser object...
      std::unique_ptr<chromium::IBrowser>   browser_;      
      std::unordered_map<std::string, luabind::object>   clientRoutes_;
      std::vector<std::function<void()>>     browserJobQueue_;
      std::mutex                             browserJobQueueLock_;

      // client side command dispatching...
      std::map<int, CommandFn>            _commands;

      // cursors...
      HCURSOR                          currentCursor_;
      HCURSOR                          uiCursor_;
      CursorMap                        cursors_;
      Cursor                           hover_cursor_;
      Cursor                           default_cursor_;
      CursorStackId                                       next_cursor_stack_id_;
      std::vector<std::pair<CursorStackId, Cursor>>       cursor_stack_;

      // server requests...
      int                              last_server_request_id_;
      std::map<int, std::function<void(tesseract::protocol::Update const& reply)> >  server_requests_;

      // server side remote object tracking...
      om::ErrorBrowserPtr                             error_browser_;

      // client side lua...
      std::unique_ptr<lua::ScriptHost>  scriptHost_;

      // for playing sounds
      //sf::SoundBuffer soundBuffer_;
      //sf::Sound       sound_;

      InputHandlerId                                           next_input_id_;
      std::vector<std::pair<InputHandlerId, InputHandlerCb>>   input_handlers_;

      // reactor...
      rpc::CoreReactorPtr         core_reactor_;
      rpc::HttpReactorPtr         http_reactor_;
      rpc::HttpDeferredPtr        get_events_deferred_;
      rpc::ProtobufRouterPtr      protobuf_router_;
      int                         mouse_x_;
      int                         mouse_y_;
      core::Guard                 guards_;
      bool                        perf_hud_shown_;

      dm::TracerSyncPtr           object_model_traces_;
      dm::TracerBufferedPtr       game_render_tracer_;
      dm::TracerBufferedPtr       authoring_render_tracer_;
      dm::ReceiverPtr             receiver_;
      dm::TracePtr                authoring_store_alloc_trace_;
      dm::TracePtr                selected_trace_;
      dm::TracePtr                root_object_trace_;
      std::shared_ptr<rpc::TraceObjectRouter> trace_object_router_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_CLIENT_H
