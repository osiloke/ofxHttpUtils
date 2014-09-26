//
//  ofxHttpResponseEvent.h
//  httpUtilsTest
//
//  Created by Osiloke Emoekpere on 8/5/14.
//
//

#ifndef __httpUtilsTest__ofxHttpResponseEvent__
#define __httpUtilsTest__ofxHttpResponseEvent__

#include "ofMain.h"
#include "ofxHttpTypes.h" 
#include <iostream>

class ofxHttpResponseEvent : public ofEventArgs {
    
public: 
    ofxHttpResponse response;
    ofxHttpRequest request;
    ofxHttpResponseEvent() {
    };
//    ofxHttpResponseEvent(ofxHttpResponse & res, ofxHttpRequest & req):response(res), request(req) {
//    };
    
//    void setResponse(ofxHttpResponse)
    static ofEvent <ofxHttpResponseEvent> events;
};
#endif /* defined(__httpUtilsTest__ofxHttpResponseEvent__) */
