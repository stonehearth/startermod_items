#include "radiant.h"
#include "radiant_macros.h"
#include "crash_reporter_server.h"
#include "build_number.h"
#include <fstream>
#include <sstream>

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
static std::string const HORDE_LOG_FILENAME = "gfx.log";

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
                              std::string const& userid,
                              std::function<void()> const& exit_process_function)
{
   pipe_name_ = pipe_name;
   dump_path_ = dump_path;
   uri_ = uri;
   userid_ = userid;
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

   // Get Horde log
   boost::filesystem::path horde_log_path = (old_dump_path.parent_path() / HORDE_LOG_FILENAME).string();

   // Get zip filename
   boost::filesystem::path zip_path(new_dump_path);
   zip_path.replace_extension(".zip");

   // Create zip package
   std::vector<std::string> files;
   files.push_back(new_dump_path.string());
   files.push_back(horde_log_path.string());
   CreateZip(zip_path.string(), files);

   // Get the length of the zip file
   std::ifstream zip_file(zip_path.string().c_str(), std::ios::in|std::ios::binary);
   zip_file.seekg(0, std::ios::end);
   long long const zip_file_length = zip_file.tellg();
   zip_file.seekg(0, std::ios::beg);

   // Set up HTTP POST
   Poco::URI uri(uri_);

   std::ostringstream query;
   query << "branch="    << PRODUCT_BRANCH;
   query << "&userid="   << userid_;
   query << "&product="  << PRODUCT_NAME;
   query << "&build="    << PRODUCT_BUILD_NUMBER;
   query << "&revision=" << PRODUCT_REVISION;
   uri.setQuery(query.str());

   std::string path(uri.getPathAndQuery());
   Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
   Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPMessage::HTTP_1_1);

   request.setContentType("application/octet-stream");
   request.setContentLength(zip_file_length);

   // Send request, returns open stream
   std::ostream& request_stream = session.sendRequest(request);
   Poco::StreamCopier::copyStream(zip_file, request_stream);
   zip_file.close();

   // Delete the zip file
   boost::filesystem::remove(zip_path);

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
void CrashReporterServer::CreateZip(std::string const& zip_filename, const std::vector<std::string>& files) const
{
   std::ofstream zip_file(zip_filename, std::ios::binary);
   Poco::Zip::Compress encoder(zip_file, true);

   for (const auto& file : files)
   {
      std::string const unq_dump_filename(boost::filesystem::path(file).filename().string());
      encoder.addFile(file, unq_dump_filename);
   }
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
   try {
   } catch (...) {
   }
}

void CrashReporterServer::OnClientCrashed(void* context, google_breakpad::ClientInfo const* client_info, std::wstring const* dump_filename_w)
{
   try {
      CrashReporterServer* crash_reporter = (CrashReporterServer*) context;
      crash_reporter->SendCrashReport(WstringToString(*dump_filename_w));

      crash_reporter->ExitProcess();
   } catch (...) {
   }
}

void CrashReporterServer::OnClientExited(void* context, google_breakpad::ClientInfo const* client_info)
{
   try {
      CrashReporterServer* crash_reporter = (CrashReporterServer*) context;
      crash_reporter->ExitProcess();
   } catch (...) {
   }
}
