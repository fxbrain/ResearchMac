//
// Created by Stefan Schwarz on 29.01.18.
//

#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include <memory>

boost::asio::io_service service(100);

class RequestHandler : public Poco::Net::HTTPRequestHandler {
public:
    RequestHandler(){}
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
    {
        std::string sOrigin, sPost, sRet;
        std::string session_id;

        if (request.has("Origin"))
        {

        }
    }
};

class RequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    RequestHandlerFactory(){}
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request)
    {
        return new RequestHandler();
    }
};

class HttpServer : public boost::enable_shared_from_this<HttpServer>{

    Poco::Net::ServerSocket svs;
    Poco::Net::HTTPServer srv;

public:
    HttpServer(std::string address_, Poco::UInt16 port_):
            svs(Poco::Net::SocketAddress(address_.empty() ? "127.0.0.1" : address_, port_)),
            srv(new RequestHandlerFactory(), svs, new Poco::Net::HTTPServerParams)
    {
        svs.setReuseAddress(true);
        svs.setReusePort(true);
    }

    virtual ~HttpServer(){}

    void start()
    {
        service.post(
                boost::bind(&HttpServer::exec, shared_from_this()));
    }

    void stop()
    {
        srv.stop();
    }

private:
    void exec()
    {
        srv.start();
    }
};

int main() {
    struct App : Poco::Util::ServerApplication {
        ~App() {
            waitForTerminationRequest();
        }
    } app;

    for (int i=0; i<2; ++i) {
        boost::make_shared<HttpServer>("", 8080+i)->start();
    }
}