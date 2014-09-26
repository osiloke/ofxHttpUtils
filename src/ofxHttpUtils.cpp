/*
    ofxHttpUtils v0.3
    Chris O'Shea, Arturo, Jesus, CJ

    Modified: 16th March 2009
    openFrameworks 0.06

*/

#undef verify 

#include "Poco/Net/SSLManager.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/FilePartSource.h"
#include "Poco/Net/StringPartSource.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/HTTPSession.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"

#include "ofxHttpUtils.h"
#include "ofEvents.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

bool ofxHttpUtils::initialized = false;

// ----------------------------------------------------------------------
ofxHttpUtils::ofxHttpUtils(){
    timeoutSeconds = 2;
    verbose = true;
    sendCookies = true;
    //start();

    if(!initialized){
    	SharedPtr<PrivateKeyPassphraseHandler> pConsoleHandler = new KeyConsoleHandler(false);
    	SharedPtr<InvalidCertificateHandler> pInvalidCertHandler = new ConsoleCertificateHandler(true);
    	Context::Ptr pContext = new Context(Context::CLIENT_USE, "", Context::VERIFY_NONE);
    	SSLManager::instance().initializeClient(pConsoleHandler, pInvalidCertHandler, pContext);
    	initialized = true;
    }
}

// ----------------------------------------------------------------------
ofxHttpUtils::~ofxHttpUtils(){
}

// ----------------------------------------------------------------------
void ofxHttpUtils::addRequest(ofxHTTPJsonRequest req){
	lock();
    requests.push(req);
    condition.signal();
    unlock();
}

// ----------------------------------------------------------------------
void ofxHttpUtils::start() {
    startThread(true, false);
}

// ----------------------------------------------------------------------
void ofxHttpUtils::stop() {
    stopThread();
    condition.signal();
}

// ----------------------------------------------------------------------
void ofxHttpUtils::threadedFunction(){
	lock();
    while( isThreadRunning() ){
        //Check if there are new requests
        if(requests.size()>0){
			ofxHTTPJsonRequest req = requests.front();
			ofxHttpResponse response;
	    	unlock();
			if(req.method==OFX_HTTP_POST){
				response = doPostRequest(req);
				ofLogVerbose("ofxHttpUtils") << "(thread running) req submitted (post): "  << req.name;
			}else{
				ofLogVerbose("ofxHttpUtils") << "request submitted (get):" << req.name;
				response = getUrl(req);
			}
    		lock();
			if(response.status!=-1){
                requests.pop();
                static ofxHttpResponseEvent newEvent;
                newEvent.response = response;
                newEvent.request = req;
                responses.push(newEvent);
            }
    	}
    	if(requests.empty()){
    	    ofLogVerbose("ofxHttpUtils") << "empty, waiting";
    		condition.wait(mutex);
    	}
    }
    unlock();
    ofLogVerbose("ofxHttpUtils") << "thread finished";
}

// ----------------------------------------------------------------------
int ofxHttpUtils::getQueueLength(){
    Poco::ScopedLock<ofMutex> lock(mutex);
    return requests.size();
}

// ----------------------------------------------------------------------
void ofxHttpUtils::clearQueue(){
    Poco::ScopedLock<ofMutex> lock(mutex);
    while(!requests.empty()) requests.pop();
}

// ----------------------------------------------------------------------
string ofxHttpUtils::generateRequestUrl(ofxHTTPJsonRequest & req) {
    // url to send to
    string url = req.action;
    
    // do we have any form fields?
    int numfields = req.formIds.size();
    if(numfields > 0){
        url += "?";
        for(int i=0;i<numfields;i++){
            url += req.formIds[i] +"="+ req.formValues[i];
            if(i<numfields-1)
                url += "&";
        }
    }
    return url;
}

// ----------------------------------------------------------------------
ofxHttpResponse ofxHttpUtils::doPostRequest(ofxHTTPJsonRequest & req){
	ofxHttpResponse response;
    try{
        URI uri( req.action.c_str() );
        std::string path(uri.getPathAndQuery());
        if (path.empty()) path = "/";
        
        //HTTPClientSession session(uri.getHost(), uri.getPort());
		HTTPRequest request(HTTPRequest::HTTP_PUT, path, HTTPMessage::HTTP_1_1);
		if(auth.getUsername()!="") auth.authenticate(request);
        
		if(sendCookies){
			for(unsigned i=0; i<cookies.size(); i++){
				NameValueCollection reqCookies;
				reqCookies.add(cookies[i].getName(),cookies[i].getValue());
				request.setCookies(reqCookies);
			}
		}
        
        HTTPResponse res;
        
        ofPtr<HTTPSession> session;
        istream * rs;
        
        request.setContentType("application/json");
        request.setKeepAlive(true);
        std::string reqBody(req.body());
                            
        if(uri.getScheme()=="https"){
        	HTTPSClientSession * httpsSession = new HTTPSClientSession(uri.getHost(), uri.getPort());//,context);
//        	httpsSession->setTimeout(Poco::Timespan(0,0));
            
            request.setContentLength( reqBody.length() );
            httpsSession->sendRequest(request) << reqBody;
            
        	rs = &httpsSession->receiveResponse(res);
        	session = ofPtr<HTTPSession>(httpsSession);
        }else{
        	HTTPClientSession * httpSession = new HTTPClientSession(uri.getHost(), uri.getPort());
//        	httpSession->setTimeout(Poco::Timespan(0,0));
            
            request.setContentLength( reqBody.length() );
            httpSession->sendRequest(request) << reqBody;
            
        	rs = &httpSession->receiveResponse(res);
        	session = ofPtr<HTTPSession>(httpSession);
        }
        
		response = ofxHttpResponse(res, *rs, req.action);
        
		if(sendCookies){
			cookies.insert(cookies.begin(),response.cookies.begin(),response.cookies.end());
		}
        
		if(response.status>=300 && response.status<400){
			Poco::URI uri(request.getURI());
			uri.resolve(res.get("Location"));
			response.location = uri.toString();
		}
        if(isThreadRunning()){
            static ofxHttpResponseEvent newEvent;
            newEvent.response = response;
            newEvent.request = req;
//            ofNotifyEvent(ofxHttpResponseEvent::events, newEvent, this);
        }
        
        
    }catch (Exception& exc){
        
    	ofLogError("ofxHttpUtils") << "ofxHttpUtils error doPostForm--";
        
        //ofNotifyEvent(notifyNewError, "time out", this);
        
        // for now print error, need to broadcast a response
    	ofLogError("ofxHttpUtils") << exc.displayText();
        response.status = -1;
        response.reasonForStatus = exc.displayText();
        
    }
    
    return response;
}

// ----------------------------------------------------------------------
ofxHttpResponse ofxHttpUtils::getUrl(ofxHTTPJsonRequest req){
   ofxHttpResponse response;
   string url = generateRequestUrl(req);
    
   try{
       
		URI uri(url.c_str());
		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";

		HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);

		if(auth.getUsername()!="") auth.authenticate(request);

        if(sendCookies){
        	for(unsigned i=0; i<cookies.size(); i++){
        		NameValueCollection reqCookies;
        		reqCookies.add(cookies[i].getName(),cookies[i].getValue());
        		request.setCookies(reqCookies);
        	}
        }

		HTTPResponse res;
        ofPtr<HTTPSession> session;
        istream * rs;
        if(uri.getScheme()=="https"){
        	HTTPSClientSession * httpsSession = new HTTPSClientSession(uri.getHost(), uri.getPort());//,context);
        	httpsSession->setTimeout(Poco::Timespan(timeoutSeconds,0));
        	httpsSession->sendRequest(request);
        	rs = &httpsSession->receiveResponse(res);
        	session = ofPtr<HTTPSession>(httpsSession);
        }else{
        	HTTPClientSession * httpSession = new HTTPClientSession(uri.getHost(), uri.getPort());
        	httpSession->setTimeout(Poco::Timespan(timeoutSeconds,0));
        	httpSession->sendRequest(request);
        	rs = &httpSession->receiveResponse(res);
        	session = ofPtr<HTTPSession>(httpSession);
        }

        response=ofxHttpResponse(res, *rs, url); 

		if(sendCookies){
			cookies.insert(cookies.begin(),response.cookies.begin(),response.cookies.end());
		}

		if(response.status>=300 && response.status<400){
			Poco::URI uri(request.getURI());
			uri.resolve(res.get("Location"));
			response.location = uri.toString();
		}
        static ofxHttpResponseEvent newEvent;
        newEvent.response = response;
        newEvent.request = req;
        ofNotifyEvent(ofxHttpResponseEvent::events, newEvent, this);

		//std::cout << res.getStatus() << " " << res.getReason() << std::endl;
		//StreamCopier::copyStream(rs, std::cout);

	}catch (Exception& exc){
		ofLogError("ofxHttpUtils") << exc.displayText();
        response.status = -1;
        response.reasonForStatus = exc.displayText();
	}
	return response;

}

// ----------------------------------------------------------------------
void ofxHttpUtils::sendReceivedCookies(){
	sendCookies = true;
}

// ----------------------------------------------------------------------
void ofxHttpUtils::setBasicAuthentication(string user, string password){
	auth.setUsername(user);
	auth.setPassword(password);
}

