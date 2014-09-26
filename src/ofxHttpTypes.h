/*
    ofxHttpUtils v0.3
    Chris O'Shea, Arturo, Jesus, CJ

    Modified: 16th March 2009
    openFrameworks 0.06

*/
#ifndef OFX_HTTP_TYPES
#define OFX_HTTP_TYPES


#define OFX_HTTP_GET  0
#define OFX_HTTP_POST 1
#define OFX_HTTP_PUT 2

#include "ofUtils.h"
#include "ofxJSONElement.h"


#include "Poco/Mutex.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Condition.h"
#include "Poco/Net/HTTPBasicCredentials.h"

#include "ofThread.h"
#include "ofConstants.h"

struct ofxHttpResponse{
	ofxHttpResponse(Poco::Net::HTTPResponse& pocoResponse, std::istream &bodyStream, string turl, bool binary=false){
		status=pocoResponse.getStatus();
		try{
			timestamp=pocoResponse.getDate();
		}catch(Poco::Exception & exc){
            
		}
		reasonForStatus=pocoResponse.getReasonForStatus(pocoResponse.getStatus());
		contentType = pocoResponse.getContentType();
		responseBody.set(bodyStream);
        
        url = turl;
        pocoResponse.getCookies(cookies);
	}
    
	ofxHttpResponse(){}
    
	string getURLFilename(){
		return url.substr(url.rfind('/')+1);
	}
    
	int status; 				// return code for the response ie: 200 = OK
	string reasonForStatus;		// text explaining the status
	ofBuffer responseBody;		// the actual response
	string contentType;			// the mime type of the response
	Poco::Timestamp timestamp;		// time of the response
	string url;
	vector<Poco::Net::HTTPCookie> cookies;
	string location; 
};

class ofxHttpRequest {
private:
    
public:
    int method;
	string action;
	string name;
	string body_raw;
    vector <string> formIds;
    
	vector <string> formValues;
	std::map<string,string> formFiles;
    
	bool expectBinaryResponse;
    
    ofxHttpRequest(){
    	method = OFX_HTTP_GET;
    	expectBinaryResponse = false;
    }
    
    // ----------------------------------------------------------------------
	void addFormField(string id, string value){
        formIds.push_back( id );
        formValues.push_back( value );
	}
	// ----------------------------------------------------------------------
	void clearFormFields(){
	    formIds.clear();
        formValues.clear();
        formFiles.clear();
	}
	// ----------------------------------------------------------------------
	void addFile(string fieldName, string path){
		formFiles[fieldName] = ofToDataPath(path);
	}
    
	string getFieldValue(string id){
		for(unsigned int i=0;i<formIds.size();i++){
			if(formIds[i]==id) return formValues[i];
		}
		return "";
	}
    
//    virtual string body(){
//        body_raw = this->getBody();
//        return body_raw;
//    }
//    virtual string getBody(){ return "";};
//    
    ~ofxHttpRequest(){
    }
};

class ofxHTTPJsonRequest: public ofxHttpRequest{
public:
    ofxJSONElement data;
    
    ofxHTTPJsonRequest(){}
    string body(){
        body_raw = this->getBody();
        return body_raw;
    }
    string getBody(){
        return data.getRawString();
    }
    ~ofxHTTPJsonRequest(){}
};

class ofxHTTPRawRequest: public ofxHttpRequest{
    string data;
    
    ofxHTTPRawRequest():ofxHttpRequest(){
        
    }
    
    string getBody(){
        return data;
    }
};

struct ofxHttpForm{


	int method;
	string action;
	string name;

    ofxHttpForm(){
    	method = OFX_HTTP_GET;
    	expectBinaryResponse = false;
    }
    ~ofxHttpForm(){
        clearFormFields();
    }

	// ----------------------------------------------------------------------
	void addFormField(string id, string value){
        formIds.push_back( id );
        formValues.push_back( value );
	}
	// ----------------------------------------------------------------------
	void clearFormFields(){
	    formIds.clear();
        formValues.clear();
        formFiles.clear();
	}
	// ----------------------------------------------------------------------
	void addFile(string fieldName, string path){
		formFiles[fieldName] = ofToDataPath(path);
	}

	string getFieldValue(string id){
		for(unsigned int i=0;i<formIds.size();i++){
			if(formIds[i]==id) return formValues[i];
		}
		return "";
	}

	vector <string> formIds;
	vector <string> formValues;
	std::map<string,string> formFiles;
	bool expectBinaryResponse;
};



#endif
