#include "crash_reporter_server.h"
#include <fstream>

#include <boost/filesystem.hpp>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/StreamCopier.h"
#include "Poco/Zip/Compress.h"

// google_breakpad headers
#include "client/windows/crash_generation/client_info.h" 
#include "client/windows/crash_generation/crash_generation_server.h"

using namespace radiant;
using namespace radiant::crash_reporter::server;

static std::string const CRASH_DUMP_FILENAME = "crash.dmp";

DEFINE_SINGLETON(CrashReporterServer);

// Silly helper methods...
static std::wstring StringToWstring(std::string const& input_string)
{
   return std::wstring(input_string.begin(), input_string.end());
}

static std::string WstringToString(std::wstring const& input_string)
{
   return std::string(input_string.begin(), input_string.end());
}

void CrashReporterServer::Run(std::string const& pipe_name, std::string const& dump_path, std::string const& uri,
                              std::function<void()> const& exit_process_function)
{
   pipe_name_ = pipe_name;
   dump_path_ = dump_path;
   uri_ = uri;
   exit_process_function_ = exit_process_function;

   bool success = StartCrashGenerationServer();
   if (!success) {
      throw std::exception("CrashGenerationServer failed to start");
   }
}

bool CrashReporterServer::StartCrashGenerationServer()
{
   std::wstring const pipe_name_w = StringToWstring(pipe_name_);
   std::wstring const dump_path_w = StringToWstring(dump_path_);

   crash_server_.reset(new google_breakpad::CrashGenerationServer(
                                            pipe_name_w,       // name of the pipe
                                            nullptr,           // default pipe security
                                            OnClientConnected, // callback on client connection
                                            this,              // context for the client connection callback
                                            OnClientCrashed,   // callback on client crash
                                            this,              // context for the client crashed callback
                                            OnClientExited,    // callback on client exit
                                            this,              // context for the client exited callback
                                            nullptr,           // callback for upload request
                                            nullptr,           // context for the upload request callback
                                            true,              // generate a dump on crash
                                            &dump_path_w));    // path to place dump files
                                                               // fully qualified filename will be passed to the client crashed callback
                                                               // multiple files may be generated in the case of a full memory dump request (currently off)
   return crash_server_->Start();
}

void CrashReporterServer::SendCrashReport(std::string const& dump_filename)
{
   // Rename dump file
   boost::filesystem::path old_dump_path(dump_filename);
   boost::filesystem::path new_dump_path = (old_dump_path.parent_path() / CRASH_DUMP_FILENAME).string();
   boost::filesystem::remove(new_dump_path);
   boost::filesystem::rename(old_dump_path, new_dump_path);

   // Get zip filename
   boost::filesystem::path zip_path(new_dump_path);
   zip_path.replace_extension(".zip");

   // Create zip package
   CreateZip(zip_path.string(), new_dump_path.string());

   // Get the length of the zip file
   std::ifstream zip_file(zip_path.string().c_str(), std::ios::in|std::ios::binary);
   zip_file.seekg(0, std::ios::end);
   long long const zip_file_length = zip_file.tellg();
   zip_file.seekg(0, std::ios::beg);

   // Set up HTTP POST
   Poco::URI uri(uri_);
   std::string path(uri.getPathAndQuery());
   Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
   Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPMessage::HTTP_1_1);

   request.setContentType("application/octet-stream");
   request.setContentLength(zip_file_length);

   // Send request, returns open stream
   std::ostream& request_stream = session.sendRequest(request);
   Poco::StreamCopier::copyStream(zip_file, request_stream);
   zip_file.close();

   // Get response
   Poco::Net::HTTPResponse response;
   std::istream& response_stream = session.receiveResponse(response);

   // Check result
   int status = response.getStatus();
   if (status != Poco::Net::HTTPResponse::HTTP_OK) {
      // unexpected result code, not much we can do
	}

   // For debugging
   //std::string response_string(1000, '\0');
   //response_stream.read(&response_string[0], 1000);
   //MessageBox(nullptr, response_string.c_str(), "crash_reporter", MB_OK);
}

// Extend this to package a list of files for submission
void CrashReporterServer::CreateZip(std::string const& zip_filename, std::string const& dump_filename)
{
   std::ofstream zip_file(zip_filename, std::ios::binary);
   Poco::Zip::Compress encoder(zip_file, true);

   std::string const unq_dump_filename(boost::filesystem::path(dump_filename).filename().string());
   encoder.addFile(dump_filename, unq_dump_filename);
   // Add additional files here
   encoder.close();
}

// Must exit gracefully and allow the CrashGenerationServer a chance to clean up.
// The CrashGenerationServer still needs to signal back to the main process that the dump is complete.
// The main process is holding the process open so we can read its state before exiting.
void CrashReporterServer::ExitProcess()
{
   exit_process_function_();
}

// Static callbacks for Breakpad
void CrashReporterServer::OnClientConnected(void* context, google_breakpad::ClientInfo const* client_info)
{
}

void CrashReporterServer::OnClientCrashed(void* context, google_breakpad::ClientInfo const* client_info, std::wstring const* dump_filename_w)
{
   CrashReporterServer* crash_reporter = (CrashReporterServer*) context;
   crash_reporter->SendCrashReport(WstringToString(*dump_filename_w));

   crash_reporter->ExitProcess();
}

void CrashReporterServer::OnClientExited(void* context, google_breakpad::ClientInfo const* client_info)
{
   CrashReporterServer* crash_reporter = (CrashReporterServer*) context;
   crash_reporter->ExitProcess();
}
