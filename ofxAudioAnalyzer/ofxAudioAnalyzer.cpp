/*
 * Copyright (C) 2016 Leo Zimmerman [http://www.leozimmerman.com.ar]
 *
 * ofxAudioAnalyzer is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation (FSF), either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the Affero GNU General Public License
 * version 3 along with this program.  If not, see http://www.gnu.org/licenses/
 *
 * ---------------------------------------------------------------
 *
 * This project uses Essentia, copyrighted by Music Technology Group - Universitat Pompeu Fabra
 * using GNU Affero General Public License.
 * See http://essentia.upf.edu for documentation.
 *
 */

#include "ofxAudioAnalyzer.h"

//-------------------------------------------------------
void ofxAudioAnalyzer::setup(int sampleRate, int bufferSize, int channels){
    
    _samplerate = sampleRate;
    _buffersize = bufferSize;
    _channels = channels;
    
    if(_channels <= 0){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channels cant be set to none. Setting 1 channel");
        _channels = 1;
    }
    
    if (!essentia::isInitialized()){
        essentia::init();
    }
    
    for(int i=0; i<_channels; i++){
        ofxAudioAnalyzerUnit * aaUnit = new ofxAudioAnalyzerUnit(_samplerate, _buffersize);
        channelAnalyzerUnits.push_back(aaUnit);
    }
}
//-------------------------------------------------------
void ofxAudioAnalyzer::reset(int sampleRate, int bufferSize, int channels){
    
    _samplerate = sampleRate;
    _buffersize = bufferSize;
    _channels = channels;
    
    if(_channels <= 0){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channels cant be set to none. Setting 1 channel");
        _channels = 1;
    }
    
    for (int i=0; i<channelAnalyzerUnits.size(); i++){
        channelAnalyzerUnits[i]->exit();
    }
    channelAnalyzerUnits.clear();
    
    for(int i=0; i<_channels; i++){
        ofxAudioAnalyzerUnit * aaUnit = new ofxAudioAnalyzerUnit(_samplerate, _buffersize);
        channelAnalyzerUnits.push_back(aaUnit);
    }
    
    loadStoredMaxEstimatedValues();
}
//-------------------------------------------------------
void ofxAudioAnalyzer::analyze(const juce::AudioBuffer<float>& buffer){
   
    if(buffer.getNumChannels() != _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: inBuffer channels number incorrect.");
        return;
    }
    
    if(channelAnalyzerUnits.size()!= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: wrong number of audioAnalyzerUnits");
        return;
    }
    
    
    for (int i=0; i<_channels; i++){
        const float * channelPtr = buffer.getReadPointer(i);
        vector<float> bufferCopy;
        
        for(int i=0; i<buffer.getNumSamples();++i) {
            bufferCopy.push_back(channelPtr[i]);
        }
        if(channelAnalyzerUnits[i]!=nullptr){
            channelAnalyzerUnits[i]->analyze(bufferCopy);
        }else{
            juce::Logger::outputDebugString("ofxAudioAnalyzer: channelAnalyzer NULL pointer");
        }
    }
}
//-------------------------------------------------------
void ofxAudioAnalyzer::exit(){
    AlgorithmFactory& factory = AlgorithmFactory::instance();
    factory.shutdown();
    
    essentia::shutdown();
    
    for(int i=0; i<channelAnalyzerUnits.size();i++){
        channelAnalyzerUnits[i]->exit();
    }
}
//-------------------------------------------------------
float ofxAudioAnalyzer::getValue(ofxAAValue valueType, int channel, float smooth, bool normalized) const {
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for getting value is incorrect.");
        return 0.0;
    }
    return channelAnalyzerUnits[channel]->getValue(valueType, smooth, normalized);
}
//-------------------------------------------------------
float ofxAudioAnalyzer::getAverageValue(ofxAAValue valueType, float smooth, bool normalized) const {
    auto size = channelAnalyzerUnits.size();
    if (size <= 0){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for getting value is incorrect.");
        return 0.0;
    }
    float value = 0.0;
    for (int i=0; i<size; i++) {
        value += channelAnalyzerUnits[i]->getValue(valueType, smooth, normalized);
    }
    value /= size;
    return value;
}
//-------------------------------------------------------
vector<float>& ofxAudioAnalyzer::getValues(ofxAABinsValue valueType, int channel, float smooth, bool normalized){
    
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for getting value is incorrect.");
        static vector<float>r (1, 0.0);
        return r;
    }
    
    return channelAnalyzerUnits[channel]->getValues(valueType, smooth, normalized);
    
}
//-------------------------------------------------------
bool ofxAudioAnalyzer::getOnsetValue(int channel) const {
    
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for getting value is incorrect.");
        return false;
    }
    return channelAnalyzerUnits[channel]->getValue(ONSETS, 0.0, false);
}

//-------------------------------------------------------
void ofxAudioAnalyzer::resetOnsets(int channel){
    
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for getting value is incorrect.");
        return;
    }
    
    channelAnalyzerUnits[channel]->getOnsetsPtr()->reset();
}
//-------------------------------------------------------
void ofxAudioAnalyzer::setMaxEstimatedValue(int channel, ofxAAValue valueType, float value){
    
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for setting max estimated value is incorrect.");
        return;
    }
    
    channelAnalyzerUnits[channel]->setMaxEstimatedValue(valueType, value);
    storedMaxEstimatedValues[valueType] = value;
}
//-------------------------------------------------------
void ofxAudioAnalyzer::setMaxEstimatedValue(int channel, ofxAABinsValue valueType, float value){
    
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for setting max estimated value is incorrect.");
        return;
    }
    
    channelAnalyzerUnits[channel]->setMaxEstimatedValue(valueType, value);
}
//-------------------------------------------------------
void ofxAudioAnalyzer::setOnsetsParameters(int channel, float alpha, float silenceTresh, float timeTresh, bool useTimeTresh){
    
    if (channel >= _channels){
        juce::Logger::outputDebugString("ofxAudioAnalyzer: channel for getting value is incorrect.");
        return;
    }
    auto onsets =  channelAnalyzerUnits[channel]->getOnsetsPtr();
    onsets->setOnsetAlpha(alpha);
    onsets->setOnsetSilenceThreshold(silenceTresh);
    onsets->setOnsetTimeThreshold(timeTresh);
    onsets->setUseTimeThreshold(useTimeTresh);
}
//-------------------------------------------------------
void ofxAudioAnalyzer::loadStoredMaxEstimatedValues() {
    map<ofxAAValue, float>::iterator it;
    for(it=storedMaxEstimatedValues.begin(); it!=storedMaxEstimatedValues.end(); ++it) {
        for(int ch=0; ch<_channels; ch++){
            setMaxEstimatedValue(ch, it->first, it->second);
        }
    }
}




















