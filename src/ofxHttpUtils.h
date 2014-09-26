/*
    ofxHttpUtils v0.3
    Chris O'Shea, Arturo, Jesus, CJ

    Modified: 16th March 2009
    openFrameworks 0.06

*/

#ifndef _OFX_HTTP_UTILS
#define _OFX_HTTP_UTILS


#include <iostream>
#include <queue>
#include <istream>

#include "Poco/Mutex.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Condition.h"
#include "Poco/Net/HTTPBasicCredentials.h"

#include "ofThread.h"
#include "ofConstants.h"
#include "ofxHttpTypes.h"
#include "ofEvents.h"

#include "ofxHttpResponseEvent.h"
#include "ofxJSONElement.h"


class ofxHttpListener;
class ofxHttpEventManager;


class ofxHttpUtils : public ofThread{

	public:

		ofxHttpUtils();
		~ofxHttpUtils();
		//-------------------------------
		// non blocking functions
 
        void addRequest(ofxHTTPJsonRequest req); 

		//-------------------------------
		// blocking functions 
		ofxHttpResponse getUrl(ofxHTTPJsonRequest req);

        int getQueueLength();
        void clearQueue();

		//-------------------------------
		// threading stuff
		void threadedFunction();

		//------------------------------
		// events
		ofEvent<ofxHttpResponse> newResponseEvent;

        // other stuff-------------------
        void setTimeoutSeconds(int t){
            timeoutSeconds = t;
        }
        void setVerbose(bool v){
            verbose = v;
        }

        void sendReceivedCookies();

        void setBasicAuthentication(string user, string password);


		void start();
        void stop();
    
        ofxHttpResponseEvent getResponse(){
            newData = false;
//            if (lock()){
                if(!responses.empty()){
                    response = responses.front();
                    responses.pop();
                    newData = true;
                };
//                unlock();
//            }
            if (newData){
                return response;
            }
            else
            {
                throw 0;
            };
        }; 
    
        ofxHttpResponseEvent response;
        queue<ofxHttpResponseEvent, list<ofxHttpResponseEvent> > responses;
    
    protected:
        bool newData;
		bool verbose;
        int timeoutSeconds;
        bool sendCookies;

		//--------------------------------
		// http utils
        string generateRequestUrl(ofxHTTPJsonRequest & req); 
        ofxHttpResponse doPostRequest(ofxHTTPJsonRequest & req);

        std::queue<ofxHTTPJsonRequest> requests;
		vector<Poco::Net::HTTPCookie> cookies;
		Poco::Net::HTTPBasicCredentials auth;
		Poco::Condition condition;

		static bool initialized;

};
#endif
