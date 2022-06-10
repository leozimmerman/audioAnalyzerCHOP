//
//  ofxAAUtils.cpp
//  Sonoscopio
//
//  Created by Leo on 11/09/2018.
//

//#include "ofxAAUtils.h"
#include "StringUtils.h"

namespace utils {
    const char* valueTypeToString(ofxAAValue value) {
        for (auto const& pair : valuesMap){
            if (pair.second == value){
                return pair.first;
            }
        }
        return "-";
    }
    
    const char* binsValueTypeToString(ofxAABinsValue value){
        for (auto const& pair : binValuesMap){
            if (pair.second == value){
                return pair.first;
            }
        }
        return "-";
    }
    
    ofxAAValue stringToValueType(string stringType){
        for (auto const& pair : valuesMap){
            if (pair.first == stringType){
                return pair.second;
            }
        }
        return NONE;
    }
    
    ofxAABinsValue stringToBinsValueType(string stringType) {
        for (auto const& pair : binValuesMap){
            if (pair.first == stringType){
                return pair.second;
            }
        }
        return NONE_BINS;
    }
    
};
